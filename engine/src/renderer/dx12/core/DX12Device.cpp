#include "DX12Device.h"
#include <stdexcept>
#include <sstream>

DX12Device::~DX12Device() {
    // Explicit cleanup in reverse order of creation
    m_device.Reset();
    m_adapter.Reset();
    m_factory.Reset();

    if (m_debugController) {
        m_debugController.Reset();
    }
}

bool DX12Device::Initialize(bool enableDebugController) {
    HRESULT hr;
    UINT createFactoryFlags = 0;
    m_debugEnabled = false;

#ifdef _DEBUG
    // Enable debug layer FIRST, before any D3D12 calls
    if (enableDebugController) {
        hr = D3D12GetDebugInterface(IID_PPV_ARGS(&m_debugController));
        if (SUCCEEDED(hr)) {
            m_debugController->EnableDebugLayer();
            m_debugController->SetEnableGPUBasedValidation(true);
            m_debugController->SetEnableSynchronizedCommandQueueValidation(true);
            m_debugEnabled = true;
        } else {
            printf("D3D12GetDebugInterface failed with HRESULT: 0x%08X\n", hr);
        }
        createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    hr = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&m_factory));
    if (FAILED(hr)) {
        printf("CreateDXGIFactory2 failed with HRESULT: 0x%08X\n", hr);
        return false;
    }

    // Find hardware adapter
    if (!FindHardwareAdapter()) {
        printf("FindHardwareAdapter failed - no compatible adapter found\n");
        return false;
    }

    // Create D3D12 device
    hr = D3D12CreateDevice(
        m_adapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_device)
    );
    if (FAILED(hr)) {
        printf("D3D12CreateDevice failed with HRESULT: 0x%08X\n", hr);
        return false;
    }

#ifdef _DEBUG
    // Configure debug breaks after device creation
    if (m_debugEnabled && m_device) {
        ComPtr<ID3D12InfoQueue> infoQueue;
        if (SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        }
    }
#endif

    // Required features
    DX12Features requested;
    requested.features
        .set(DX12Features::RAY_TRACING)
        .set(DX12Features::MESH_SHADERS);

    bool hasRequired = SupportsFeatures(requested);
    if (!hasRequired) {
        std::ostringstream errorMsg;
        errorMsg << "Your GPU does not support the required features:\n\n";

        if (!SupportsFeature(DX12Features::RAY_TRACING)) {
            errorMsg << "  * Ray Tracing\n";
        }
        if (!SupportsFeature(DX12Features::MESH_SHADERS)) {
            errorMsg << "  * Mesh Shaders\n";
        }

        errorMsg << "\nPlease update your graphics drivers or use a compatible GPU.";

        throw std::runtime_error(errorMsg.str());
    }

    return true;
}

bool DX12Device::FindHardwareAdapter() {
    ComPtr<IDXGIAdapter1> adapter;

    for (UINT adapterIndex = 0;
         SUCCEEDED(m_factory->EnumAdapters1(adapterIndex, &adapter));
         ++adapterIndex) {

        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        // Skip software adapters
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }

        // Try to create device with this adapter
        HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
        if (SUCCEEDED(hr)) {
            m_adapter = adapter;
            return true;
        } else {
            printf("D3D12CreateDevice test failed for adapter %u with HRESULT: 0x%08X\n", adapterIndex, hr);
        }
    }

    return false;
}

bool DX12Device::SupportsFeatures(const DX12Features& requestedFeatures) const {
    if (!m_device) {
        return false;
    }

    // Check RayTracing
    if (requestedFeatures.features[DX12Features::RAY_TRACING]) {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
        HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
        if (FAILED(hr)) {
            printf("CheckFeatureSupport for Ray Tracing failed with HRESULT: 0x%08X\n", hr);
            return false;
        }
        if (options5.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED) {
            return false;
        }
    }

    // Check Mesh Shaders
    if (requestedFeatures.features[DX12Features::MESH_SHADERS]) {
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
        HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));
        if (FAILED(hr)) {
            printf("CheckFeatureSupport for Mesh Shaders failed with HRESULT: 0x%08X\n", hr);
            return false;
        }
        if (options7.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED) {
            return false;
        }
    }

    // Check Variable Rate Shading
    if (requestedFeatures.features[DX12Features::VARIABLE_RATE_SHADING]) {
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
        HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6));
        if (FAILED(hr)) {
            printf("CheckFeatureSupport for Variable Rate Shading failed with HRESULT: 0x%08X\n", hr);
            return false;
        }
        if (options6.VariableShadingRateTier == D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED) {
            return false;
        }
    }

    // Check Sampler Feedback
    if (requestedFeatures.features[DX12Features::SAMPLER_FEEDBACK]) {
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
        HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));
        if (FAILED(hr)) {
            printf("CheckFeatureSupport for Sampler Feedback failed with HRESULT: 0x%08X\n", hr);
            return false;
        }
        if (options7.SamplerFeedbackTier == D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED) {
            return false;
        }
    }

    return true;
}

bool DX12Device::SupportsFeature(DX12Features::FLAG feature) const {
    if (!m_device) {
        return false;
    }

    switch (feature) {
    case DX12Features::RAY_TRACING: {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
        HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
        if (FAILED(hr)) {
            printf("CheckFeatureSupport for Ray Tracing failed with HRESULT: 0x%08X\n", hr);
            return false;
        }
        return options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
    }
    case DX12Features::MESH_SHADERS: {
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
        HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));
        if (FAILED(hr)) {
            printf("CheckFeatureSupport for Mesh Shaders failed with HRESULT: 0x%08X\n", hr);
            return false;
        }
        return options7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
    }
    case DX12Features::VARIABLE_RATE_SHADING: {
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
        HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6));
        if (FAILED(hr)) {
            printf("CheckFeatureSupport for Variable Rate Shading failed with HRESULT: 0x%08X\n", hr);
            return false;
        }
        return options6.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED;
    }
    case DX12Features::SAMPLER_FEEDBACK: {
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
        HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));
        if (FAILED(hr)) {
            printf("CheckFeatureSupport for Sampler Feedback failed with HRESULT: 0x%08X\n", hr);
            return false;
        }
        return options7.SamplerFeedbackTier != D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED;
    }
    default:
        return false;
    }
}

// Getters
ID3D12Device* DX12Device::GetDevice() const {
    return m_device.Get();
}

IDXGIFactory4* DX12Device::GetFactory() const {
    return m_factory.Get();
}

IDXGIAdapter1* DX12Device::GetAdapter() const {
    return m_adapter.Get();
}

uint32_t DX12Device::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const {
    return m_device->GetDescriptorHandleIncrementSize(type);
}
