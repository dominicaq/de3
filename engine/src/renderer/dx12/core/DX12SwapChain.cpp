#include "DX12SwapChain.h"
#include "Config.h"
#include <stdexcept>
#include <string>

SwapChain::SwapChain(IDXGIFactory4* factory, ID3D12CommandQueue* commandQueue, HWND hwnd, const EngineConfig& config)
    : m_commandQueue(commandQueue)
    , m_hwnd(hwnd)
    , m_config(config)
    , m_format(DXGI_FORMAT_R8G8B8A8_UNORM)
    , m_currentBackBufferIndex(0)
    , m_bufferCount(config.maxFramesInFlight) {

    if (!factory) {
        throw std::invalid_argument("Factory cannot be null");
    }

    if (!m_commandQueue) {
        throw std::invalid_argument("Command queue cannot be null");
    }

    // Use config for initial size, but validate with actual window size
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    UINT actualWidth = clientRect.right - clientRect.left;
    UINT actualHeight = clientRect.bottom - clientRect.top;

    // Use actual window size if available, otherwise fall back to config
    m_width = (actualWidth > 0) ? actualWidth : config.windowWidth;
    m_height = (actualHeight > 0) ? actualHeight : config.windowHeight;

    // Ensure minimum size
    if (m_width == 0) m_width = 1;
    if (m_height == 0) m_height = 1;

    CreateSwapChain(factory);
    CreateBackBuffers();
}

void SwapChain::CreateSwapChain(IDXGIFactory4* factory) {
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = m_bufferCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = m_format;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; // For variable refresh rate

    ComPtr<IDXGISwapChain1> swapChain1;
    HRESULT hr = factory->CreateSwapChainForHwnd(
        m_commandQueue,
        m_hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1
    );

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create swap chain");
    }

    // Disable Alt+Enter fullscreen toggle
    hr = factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(hr)) {
        // Non-fatal, just continue
    }

    // QueryInterface to IDXGISwapChain3
    hr = swapChain1.As(&m_swapChain);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to cast swap chain to IDXGISwapChain3");
    }

    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void SwapChain::CreateBackBuffers() {
    // Resize the array if buffer count changed
    m_backBuffers.resize(m_bufferCount);

    for (UINT i = 0; i < m_bufferCount; i++) {
        HRESULT hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to get swap chain back buffer " + std::to_string(i));
        }

        // Set debug names for easier debugging
#ifdef _DEBUG
        std::wstring name = L"SwapChain BackBuffer " + std::to_wstring(i);
        m_backBuffers[i]->SetName(name.c_str());
#endif
    }
}

void SwapChain::ReleaseBackBuffers() {
    for (auto& buffer : m_backBuffers) {
        buffer.Reset();
    }
}

void SwapChain::Present(bool vsync) {
    UINT syncInterval = vsync ? 1 : 0;
    UINT flags = 0;

    // Only use tearing if VSync is off and tearing is supported
    if (!vsync) {
        flags = DXGI_PRESENT_ALLOW_TEARING;
    }

    HRESULT hr = m_swapChain->Present(syncInterval, flags);
    if (FAILED(hr)) {
        // Handle device lost scenarios
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            throw std::runtime_error("Device lost during present");
        }
        throw std::runtime_error("Failed to present swap chain");
    }

    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void SwapChain::Resize(UINT width, UINT height) {
    if (width == 0 || height == 0) {
        return; // Invalid size or window minimized
    }

    if (width == m_width && height == m_height) {
        return; // No change needed
    }

    // Release back buffer references
    ReleaseBackBuffers();

    // Resize the swap chain buffers
    HRESULT hr = m_swapChain->ResizeBuffers(
        m_bufferCount,
        width,
        height,
        m_format,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
    );

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to resize swap chain buffers");
    }

    m_width = width;
    m_height = height;
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Recreate back buffer references
    CreateBackBuffers();
}

ID3D12Resource* SwapChain::GetCurrentBackBuffer() const {
    if (m_currentBackBufferIndex >= m_backBuffers.size()) {
        return nullptr;
    }
    return m_backBuffers[m_currentBackBufferIndex].Get();
}
