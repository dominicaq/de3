#include "UniformManager.h"
#include <algorithm>
#include <cstdio>

UniformManager::UniformManager(D3D12MA::Allocator* allocator, ID3D12Device* device)
    : m_allocator(allocator), m_device(device) {
}

UniformManager::~UniformManager() {
    Shutdown();
}

bool UniformManager::Initialize(const Config& config) {
    if (!m_allocator || !m_device) {
        printf("UniformManager: Invalid allocator or device\n");
        return false;
    }

    m_config = config;
    m_frameCount = std::min(config.frameCount, 3u);

    if (!CreateFrameBuffers()) {
        printf("UniformManager: Failed to create frame buffers\n");
        return false;
    }

    if (!CreateDescriptorHeap()) {
        printf("UniformManager: Failed to create descriptor heap\n");
        return false;
    }

    m_descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_isInitialized = true;
    printf("UniformManager: Initialized with D3D12MA - %zu MB per frame, %u frames\n",
           m_config.frameBufferSize / (1024 * 1024), m_frameCount);

    return true;
}

void UniformManager::Shutdown() {
    if (!m_isInitialized) return;

    // Wait for GPU to finish using any resources
    // In a real engine, you'd wait on fences here

    // Release frame buffers
    for (uint32_t i = 0; i < m_frameCount; ++i) {
        FrameData& frame = m_frameData[i];
        if (frame.resource && frame.mappedData) {
            frame.resource->Unmap(0, nullptr);
            frame.mappedData = nullptr;
        }
        if (frame.allocation) {
            frame.allocation->Release();
            frame.allocation = nullptr;
        }
        frame.resource = nullptr;
    }

    m_descriptorHeap.Reset();
    m_isInitialized = false;

    printf("UniformManager: Shutdown complete\n");
}

bool UniformManager::CreateFrameBuffers() {
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = m_config.frameBufferSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;

    for (uint32_t i = 0; i < m_frameCount; ++i) {
        FrameData& frame = m_frameData[i];

        // Use D3D12MA to create the buffer
        HRESULT hr = m_allocator->CreateResource(
            &allocDesc,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            &frame.allocation,
            IID_PPV_ARGS(&frame.resource)
        );

        if (FAILED(hr)) {
            printf("UniformManager: Failed to create frame buffer %u (HRESULT: 0x%08X)\n", i, hr);
            return false;
        }

        frame.size = m_config.frameBufferSize;

        // Map the buffer (keep it mapped for the lifetime)
        D3D12_RANGE readRange = {0, 0}; // We won't read on CPU
        hr = frame.resource->Map(0, &readRange, reinterpret_cast<void**>(&frame.mappedData));
        if (FAILED(hr)) {
            printf("UniformManager: Failed to map frame buffer %u (HRESULT: 0x%08X)\n", i, hr);
            return false;
        }

        // Set debug name
        wchar_t debugName[64];
        swprintf_s(debugName, L"UniformManager Frame Buffer %u", i);
        frame.resource->SetName(debugName);
    }

    return true;
}

bool UniformManager::CreateDescriptorHeap() {
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = m_config.maxDescriptors;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.NodeMask = 0;

    HRESULT hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeap));
    if (FAILED(hr)) {
        printf("UniformManager: Failed to create descriptor heap (HRESULT: 0x%08X)\n", hr);
        return false;
    }

    m_descriptorHeap->SetName(L"UniformManager CBV Descriptor Heap");
    return true;
}

void UniformManager::BeginFrame(uint32_t frameIndex) {
    std::lock_guard<std::mutex> lock(m_allocationMutex);

    m_currentFrame = frameIndex % m_frameCount;

    // Reset the frame buffer allocation
    ResetFrameData(m_frameData[m_currentFrame]);

    // Reset descriptor allocation (simple approach - could be more sophisticated)
    m_currentDescriptorOffset = 0;
}

void UniformManager::EndFrame() {
    // Optional: Could track frame completion with fences here
    // For now, just print usage stats if very high
    size_t usage = GetCurrentFrameUsage();
    if (usage > m_config.frameBufferSize * 0.9f) {
        printf("UniformManager Warning: Frame %u used %zu/%zu bytes (%.1f%%)\n",
               m_currentFrame, usage, m_config.frameBufferSize,
               (float)usage / m_config.frameBufferSize * 100.0f);
    }
}

UniformManager::UniformHandle UniformManager::UploadUniform(const void* data, size_t size) {
    if (!m_isInitialized || !data || size == 0) {
        return {}; // Invalid handle
    }

    // Align size to 256 bytes (CBV requirement)
    size_t alignedSize = (size + 255) & ~255;

    std::lock_guard<std::mutex> lock(m_allocationMutex);

    FrameData& frame = m_frameData[m_currentFrame];

    // Check if we have space in this frame's buffer
    if (frame.currentOffset + alignedSize > frame.size) {
        printf("UniformManager: Frame buffer overflow! Used: %zu, Need: %zu, Total: %zu\n",
               frame.currentOffset, alignedSize, frame.size);
        return {}; // Invalid handle
    }

    // Copy data to mapped buffer
    memcpy(frame.mappedData + frame.currentOffset, data, size);

    // Calculate GPU address for this allocation
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = frame.resource->GetGPUVirtualAddress() + frame.currentOffset;

    // Create descriptor for this constant buffer
    uint32_t descriptorIndex = AllocateDescriptor(gpuAddress, alignedSize);
    if (descriptorIndex == UINT32_MAX) {
        printf("UniformManager: Descriptor heap overflow!\n");
        return {}; // Invalid handle
    }

    // Advance the offset for next allocation
    frame.currentOffset += alignedSize;

    return { gpuAddress, descriptorIndex };
}

uint32_t UniformManager::AllocateDescriptor(D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, size_t bufferSize) {
    if (m_currentDescriptorOffset >= m_config.maxDescriptors) {
        return UINT32_MAX; // Out of descriptors
    }

    // Calculate descriptor handle
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += m_currentDescriptorOffset * m_descriptorSize;

    // Create constant buffer view
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = gpuAddress;
    cbvDesc.SizeInBytes = static_cast<UINT>(bufferSize);

    m_device->CreateConstantBufferView(&cbvDesc, cpuHandle);

    return m_currentDescriptorOffset++;
}

void UniformManager::BindDescriptorHeap(ID3D12GraphicsCommandList* cmdList) {
    if (!cmdList || !m_isInitialized) return;

    ID3D12DescriptorHeap* heaps[] = { m_descriptorHeap.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);
}

void UniformManager::SetGraphicsRootDescriptorTable(ID3D12GraphicsCommandList* cmdList,
                                                   uint32_t rootParameterIndex,
                                                   UniformHandle handle) {
    if (!cmdList || !handle.IsValid()) return;

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    gpuHandle.ptr += handle.descriptorIndex * m_descriptorSize;

    cmdList->SetGraphicsRootDescriptorTable(rootParameterIndex, gpuHandle);
}

void UniformManager::ResetFrameData(FrameData& frameData) {
    frameData.currentOffset = 0;
    // Note: We don't need to clear the mapped memory, just reset the offset
}

void UniformManager::PrintStats() const {
    if (!m_isInitialized) return;

    printf("=== UniformManager Stats ===\n");
    printf("Frame Count: %u\n", m_frameCount);
    printf("Buffer Size per Frame: %zu MB\n", m_config.frameBufferSize / (1024 * 1024));
    printf("Max Descriptors: %u\n", m_config.maxDescriptors);
    printf("Current Frame: %u\n", m_currentFrame);

    for (uint32_t i = 0; i < m_frameCount; ++i) {
        const FrameData& frame = m_frameData[i];
        float usage = frame.size > 0 ? (float)frame.currentOffset / frame.size * 100.0f : 0.0f;
        printf("Frame %u: %zu / %zu bytes used (%.1f%%)\n",
               i, frame.currentOffset, frame.size, usage);
    }

    printf("Current Descriptor Offset: %u / %u\n", m_currentDescriptorOffset, m_config.maxDescriptors);
    printf("============================\n");
}

size_t UniformManager::GetCurrentFrameUsage() const {
    if (!m_isInitialized) return 0;
    return m_frameData[m_currentFrame].currentOffset;
}
