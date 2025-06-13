#include "ResourceManager.h"

bool ResourceManager::Initialize(DX12Device* device, ID3D12CommandQueue* commandQueue) {
    D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
    allocatorDesc.pDevice = device->GetDevice();
    allocatorDesc.pAdapter = static_cast<IDXGIAdapter*>(device->GetAdapter());
    allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;

    HRESULT hr = D3D12MA::CreateAllocator(&allocatorDesc, &m_allocator);
    if (FAILED(hr)) {
        printf("Failed to create D3D12MA allocator: 0x%08X\n", hr);
        return false;
    }
    return true;
}

void ResourceManager::Shutdown() {
    m_allocator.Reset();
}

bool ResourceManager::CreateBuffer(size_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialState,
                                  D3D12MA::Allocation** allocation, ID3D12Resource** resource) {
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = size;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = flags;

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = m_allocator->CreateResource(
        &allocDesc,
        &resourceDesc,
        initialState,
        nullptr,
        allocation,
        IID_PPV_ARGS(resource)
    );

    if (FAILED(hr)) {
        printf("Failed to create buffer: 0x%08X\n", hr);
        return false;
    }
    return true;
}

bool ResourceManager::CreateTexture2D(UINT width, UINT height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags,
                                     D3D12_RESOURCE_STATES initialState, D3D12MA::Allocation** allocation,
                                     ID3D12Resource** resource) {
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = width;
    resourceDesc.Height = height;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = format;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = flags;

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = m_allocator->CreateResource(
        &allocDesc,
        &resourceDesc,
        initialState,
        nullptr,
        allocation,
        IID_PPV_ARGS(resource)
    );

    if (FAILED(hr)) {
        printf("Failed to create texture: 0x%08X\n", hr);
        return false;
    }
    return true;
}

void ResourceManager::BeginFrame() {
}

void ResourceManager::EndFrame() {
}
