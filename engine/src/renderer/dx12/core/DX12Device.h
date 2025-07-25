#pragma once

#include <D3D12MemAlloc.h>
#include "DX12Common.h"

class DX12Device {
private:
    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGIFactory4> m_factory;
    ComPtr<IDXGIAdapter1> m_adapter;
    ComPtr<D3D12MA::Allocator> m_allocator;
    ComPtr<ID3D12Debug> m_debugController;
    ComPtr<ID3D12InfoQueue> m_infoQueue;
    bool m_debugEnabled = false;

    bool FindHardwareAdapter();

public:
    ~DX12Device();

    bool Initialize(bool enableDebugController = false);

    // Check if a feature is supported
    bool SupportsFeatures(const DX12Features& requestedFeatures) const;
    bool SupportsFeature(DX12Features::FLAG feature) const;

    // Getters
    ID3D12Device* GetD3D12Device() const { return m_device.Get(); }
    IDXGIFactory4* GetFactory() const { return m_factory.Get(); }
    IDXGIAdapter1* GetAdapter() const { return m_adapter.Get(); }
    D3D12MA::Allocator* GetAllocator() const {return m_allocator.Get(); }

    uint32_t GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const { return m_device->GetDescriptorHandleIncrementSize(type); }

    // Debug controller
    void PrintAndClearInfoQueue() const;
};
