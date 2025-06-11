#pragma once

#include "DX12Common.h"

class DX12Device {
private:
    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGIFactory4> m_factory;
    ComPtr<IDXGIAdapter1> m_adapter;
    ComPtr<ID3D12Debug> m_debugController;
    bool m_debugEnabled = false;

    bool FindHardwareAdapter();

public:
    ~DX12Device();

    bool Initialize(bool enableDebugController = false);

    // Check if all requested features are supported
    bool SupportsFeatures(const DX12Features& requestedFeatures) const;

    // Check individual feature support
    bool SupportsFeature(DX12Features::FLAG feature) const;

    // Getters
    ID3D12Device* GetDevice() const;
    IDXGIFactory4* GetFactory() const;
    IDXGIAdapter1* GetAdapter() const;
    uint32_t GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
    bool IsDebugEnabled() const { return m_debugEnabled; }
};
