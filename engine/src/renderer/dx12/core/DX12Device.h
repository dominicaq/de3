#pragma once

#include "DX12Common.h"

class DX12Device {
private:
    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGIFactory4> m_factory;
    ComPtr<IDXGIAdapter1> m_adapter;

    // Debug layer stuff
    ComPtr<ID3D12Debug> m_debugController;
    bool m_debugEnabled;

    // Private helper functions
    bool FindHardwareAdapter();

public:
    bool Initialize();
    void Shutdown();

    // Getters
    ID3D12Device* GetDevice() const;
    IDXGIFactory4* GetFactory() const;

    // Resource creation helpers
    ComPtr<ID3D12Resource> CreateBuffer(size_t size, D3D12_HEAP_TYPE heapType);
    ComPtr<ID3D12Resource> CreateTexture2D(uint32_t width, uint32_t height, DXGI_FORMAT format);
    ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);

    // Utility functions
    uint32_t GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type);
    bool CheckFeatureSupport();
};
