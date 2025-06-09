#include "DX12Device.h"

bool DX12Device::Initialize() {
    HRESULT hr;
    m_debugEnabled = false;

    // Enable debug layer in debug builds
#ifdef _DEBUG
    hr = D3D12GetDebugInterface(IID_PPV_ARGS(&m_debugController));
    if (SUCCEEDED(hr)) {
        m_debugController->EnableDebugLayer();
        m_debugEnabled = true;
    }
#endif

    // Create DXGI factory
    UINT createFactoryFlags = 0;
#ifdef _DEBUG
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    hr = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&m_factory));
    if (FAILED(hr)) {
        return false;
    }

    // Find hardware adapter
    if (!FindHardwareAdapter()) {
        return false;
    }

    // Create D3D12 device
    hr = D3D12CreateDevice(
        m_adapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_device)
    );

    if (FAILED(hr)) {
        return false;
    }

    // Check feature support (optional)
    CheckFeatureSupport();

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
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
            m_adapter = adapter;
            return true;
        }
    }

    return false;
}

void DX12Device::Shutdown() {
    // ComPtr automatically releases when reset
    m_device.Reset();
    m_adapter.Reset();
    m_factory.Reset();

    if (m_debugController) {
        m_debugController.Reset();
    }
}

// Getters
ID3D12Device* DX12Device::GetDevice() const {
    return m_device.Get();
}

IDXGIFactory4* DX12Device::GetFactory() const {
    return m_factory.Get();
}

uint32_t DX12Device::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) {
    return m_device->GetDescriptorHandleIncrementSize(type);
}

bool DX12Device::CheckFeatureSupport() {
    // Check for basic DirectX 12 support
    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));

    if (FAILED(hr)) {
        return false;
    }

    // You can add more feature checks here as needed
    return true;
}

ComPtr<ID3D12Resource> DX12Device::CreateBuffer(size_t size, D3D12_HEAP_TYPE heapType) {
    // Set up heap properties
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = heapType;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    // Set up resource description
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = size;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ComPtr<ID3D12Resource> buffer;
    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&buffer)
    );

    if (FAILED(hr)) {
        return nullptr;
    }

    return buffer;
}

ComPtr<ID3D12Resource> DX12Device::CreateTexture2D(uint32_t width, uint32_t height, DXGI_FORMAT format) {
    // Set up heap properties
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    // Set up resource description
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Alignment = 0;
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = 1;
    textureDesc.Format = format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ComPtr<ID3D12Resource> texture;
    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&texture)
    );

    if (FAILED(hr)) {
        return nullptr;
    }

    return texture;
}

ComPtr<ID3D12DescriptorHeap> DX12Device::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors) {
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = type;
    heapDesc.NumDescriptors = numDescriptors;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    // Make shader visible for CBV/SRV/UAV heaps
    if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    }

    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    HRESULT hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap));

    if (FAILED(hr)) {
        return nullptr;
    }

    return descriptorHeap;
}
