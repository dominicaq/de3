#pragma once

#include "core/DX12Common.h"
#include "core/DX12Device.h"
#include "D3D12MemAlloc.h"

class ResourceManager {
public:
    ResourceManager() = default;
    ~ResourceManager() = default;

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    bool Initialize(DX12Device* device, ID3D12CommandQueue* commandQueue);
    void Shutdown();

    bool CreateBuffer(size_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialState,
                     D3D12MA::Allocation** allocation, ID3D12Resource** resource);

    bool CreateTexture2D(UINT width, UINT height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags,
                        D3D12_RESOURCE_STATES initialState, D3D12MA::Allocation** allocation,
                        ID3D12Resource** resource);

    void BeginFrame();
    void EndFrame();

private:
    ComPtr<D3D12MA::Allocator> m_allocator;
};
