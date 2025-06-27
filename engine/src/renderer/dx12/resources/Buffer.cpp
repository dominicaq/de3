#include "Buffer.h"

Buffer::~Buffer() {
    if (m_mappedData) {
        m_buffer->Unmap(0, nullptr);
        m_mappedData = nullptr;
    }

    if (m_allocation) {
        m_allocation->Release();
        m_allocation = nullptr;
    }
}

bool Buffer::Initialize(D3D12MA::Allocator* allocator, const void* data, UINT64 size, UINT stride, bool dynamic) {
    if (!allocator || !data || size == 0 || stride == 0) {
        printf("Buffer::Initialize - Invalid parameters\n");
        return false;
    }

    m_size = size;
    m_stride = stride;
    m_isDynamic = dynamic;

    // Create resource description
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // Setup allocation based on usage
    D3D12MA::ALLOCATION_DESC allocDesc = {};
    D3D12_RESOURCE_STATES initialState;

    if (dynamic) {
        allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    } else {
        allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        initialState = D3D12_RESOURCE_STATE_COMMON; // Let caller transition as needed
    }

    // Create the buffer
    HRESULT hr = allocator->CreateResource(
        &allocDesc,
        &desc,
        initialState,
        nullptr,
        &m_allocation,
        IID_PPV_ARGS(&m_buffer)
    );

    if (FAILED(hr)) {
        printf("Failed to create buffer: 0x%08X\n", hr);
        return false;
    }

#ifdef _DEBUG
    std::wstring name = L"Buffer_" + std::to_wstring(size) + L"_bytes";
    m_buffer->SetName(name.c_str());
    m_allocation->SetName(name.c_str());
#endif

    // Handle data upload
    if (dynamic) {
        // Map dynamic buffer and copy data
        D3D12_RANGE readRange = { 0, 0 };
        hr = m_buffer->Map(0, &readRange, &m_mappedData);
        if (FAILED(hr)) {
            printf("Failed to map dynamic buffer: 0x%08X\n", hr);
            return false;
        }

        memcpy(m_mappedData, data, size);
        // Keep mapped for future updates

    } else {
        // For static buffers, we need upload via command list
        printf("Static buffer created - upload via command list needed\n");
    }

    return true;
}

bool Buffer::Update(const void* data, UINT64 size, UINT64 offset) {
    if (!m_isDynamic) {
        printf("Update: Buffer is not dynamic\n");
        return false;
    }

    if (!m_mappedData) {
        printf("Update: Buffer not mapped\n");
        return false;
    }

    if (offset + size > m_size) {
        printf("Update: Data exceeds buffer size\n");
        return false;
    }

    memcpy(static_cast<char*>(m_mappedData) + offset, data, size);
    return true;
}

D3D12_VERTEX_BUFFER_VIEW Buffer::GetVertexView() const {
    D3D12_VERTEX_BUFFER_VIEW view = {};
    view.BufferLocation = m_buffer->GetGPUVirtualAddress();
    view.SizeInBytes = static_cast<UINT>(m_size);
    view.StrideInBytes = m_stride;
    return view;
}

D3D12_INDEX_BUFFER_VIEW Buffer::GetIndexView(bool is32Bit) const {
    D3D12_INDEX_BUFFER_VIEW view = {};
    view.BufferLocation = m_buffer->GetGPUVirtualAddress();
    view.SizeInBytes = static_cast<UINT>(m_size);
    view.Format = is32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
    return view;
}
