#include "SwapChain.h"
#include <string>

SwapChain::SwapChain(DX12Device* device, ID3D12CommandQueue* commandQueue, HWND hwnd,
                     UINT width, UINT height, UINT bufferCount)
    : m_device(device)
    , m_commandQueue(commandQueue)
    , m_hwnd(hwnd)
    , m_currentBackBufferIndex(0)
    , m_bufferCount(bufferCount)
    , m_initialized(false)
    , m_width(width)
    , m_height(height) {

    if (!Initialize(width, height)) {
        return;
    }
}

SwapChain::~SwapChain() {
    ReleaseBackBuffers();
    m_rtvDescriptorHeap.Reset();
}

bool SwapChain::Initialize(UINT width, UINT height) {
    if (!m_device || !m_commandQueue) {
        return false;
    }

    if (m_bufferCount < 2 || m_bufferCount > 16) {
        return false;
    }

    // Window size validation
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    UINT actualWidth = clientRect.right - clientRect.left;
    UINT actualHeight = clientRect.bottom - clientRect.top;

    UINT finalWidth = (width == 0 && actualWidth > 0) ? actualWidth : width;
    UINT finalHeight = (height == 0 && actualHeight > 0) ? actualHeight : height;

    if (finalWidth == 0) finalWidth = 1;
    if (finalHeight == 0) finalHeight = 1;

    // Create swap chain components
    if (!CreateSwapChain(finalWidth, finalHeight)) {
        return false;
    }

    m_device->GetFactory()->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);

    if (!CreateBackBuffers() || !CreateRTVs()) {
        return false;
    }

    m_initialized = true;
    return true;
}

bool SwapChain::CreateSwapChain(UINT width, UINT height) {
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = m_bufferCount;
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    ComPtr<IDXGISwapChain1> swapChain1;
    HRESULT hr = m_device->GetFactory()->CreateSwapChainForHwnd(
        m_commandQueue, m_hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1);

    if (FAILED(hr)) {
        printf("CreateSwapChainForHwnd failed with HRESULT: 0x%08X\n", hr);
        return false;
    }

    hr = swapChain1.As(&m_swapChain);
    if (FAILED(hr)) {
        printf("SwapChain QueryInterface to IDXGISwapChain3 failed with HRESULT: 0x%08X\n", hr);
        return false;
    }

    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    return true;
}

bool SwapChain::CreateBackBuffers() {
    m_backBuffers.resize(m_bufferCount);

    for (UINT i = 0; i < m_bufferCount; i++) {
        HRESULT hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));
        if (FAILED(hr)) {
            printf("SwapChain GetBuffer(%u) failed with HRESULT: 0x%08X\n", i, hr);
            return false;
        }

#ifdef _DEBUG
        std::wstring name = L"SwapChain BackBuffer " + std::to_wstring(i);
        m_backBuffers[i]->SetName(name.c_str());
#endif
    }

    return true;
}

void SwapChain::ReleaseBackBuffers() {
    for (auto& buffer : m_backBuffers) {
        buffer.Reset();
    }
}

bool SwapChain::Present(bool vsync) {
    if (!m_initialized) {
        return false;
    }

    UINT syncInterval = vsync ? 1 : 0;
    UINT flags = vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING;

    HRESULT hr = m_swapChain->Present(syncInterval, flags);
    if (FAILED(hr)) {
        printf("SwapChain Present failed with HRESULT: 0x%08X\n", hr);
        return false;
    }

    // Update current back buffer index after present
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    return true;
}

bool SwapChain::Reconfigure(UINT width, UINT height, UINT bufferCount) {
    if (!m_initialized) {
        return false;
    }

    UINT newBufferCount = (bufferCount == 0) ? m_bufferCount : bufferCount;
    if (newBufferCount < 2 || newBufferCount > 16 || width == 0 || height == 0) {
        return false;
    }

    ReleaseBackBuffers();


    HRESULT hr = m_swapChain->ResizeBuffers(
        newBufferCount,
        width,
        height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
    );

    if (FAILED(hr)) {
        printf("SwapChain ResizeBuffers failed with HRESULT: 0x%08X\n", hr);
        return false;
    }

    m_width = width;
    m_height = height;

    // Update state
    m_bufferCount = newBufferCount;
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    return CreateBackBuffers() && CreateRTVs();
}

bool SwapChain::CreateRTVs() {
    // Release existing heap first
    m_rtvDescriptorHeap.Reset();

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = m_bufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = m_device->GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap));
    if (FAILED(hr)) {
        printf("Failed to create RTV descriptor heap: 0x%08X\n", hr);
        return false;
    }

    m_rtvDescriptorSize = m_device->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < m_bufferCount; i++) {
        m_device->GetDevice()->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;
    }

    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::GetCurrentBackBufferRTV() const {
    return GetBackBufferRTV(m_currentBackBufferIndex);
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::GetBackBufferRTV(UINT index) const {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += index * m_rtvDescriptorSize;
    return handle;
}
