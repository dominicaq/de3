#include "DX12SwapChain.h"
#include <stdexcept>
#include <string>

SwapChain::SwapChain(DX12Device* device, ID3D12CommandQueue* commandQueue, HWND hwnd,
                     UINT width, UINT height, UINT bufferCount)
    : m_device(device)
    , m_commandQueue(commandQueue)
    , m_hwnd(hwnd)
    , m_currentBackBufferIndex(0)
    , m_bufferCount(bufferCount)
    , m_frameFenceEvent(nullptr) {

    if (!m_device) {
        throw std::invalid_argument("Device cannot be null");
    }

    if (!m_commandQueue) {
        throw std::invalid_argument("Command queue cannot be null");
    }

    // Validate buffer count (DXGI requires 2-16 buffers for flip model)
    if (m_bufferCount < 2 || m_bufferCount > 16) {
        throw std::invalid_argument("Back buffer count must be between 2 and 16");
    }

    // Use provided dimensions, but validate with actual window size if needed
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    UINT actualWidth = clientRect.right - clientRect.left;
    UINT actualHeight = clientRect.bottom - clientRect.top;

    // Use actual window size if available and provided size is 0
    UINT finalWidth = (width == 0 && actualWidth > 0) ? actualWidth : width;
    UINT finalHeight = (height == 0 && actualHeight > 0) ? actualHeight : height;

    // Ensure minimum size
    if (finalWidth == 0) finalWidth = 1;
    if (finalHeight == 0) finalHeight = 1;

    CreateSwapChain(finalWidth, finalHeight);
    m_device->GetFactory()->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);
    CreateBackBuffers();
    InitializeSynchronization();
}

SwapChain::~SwapChain() {
    // Wait for all frames to complete before destruction
    WaitForAllFrames();

    if (m_frameFenceEvent) {
        CloseHandle(m_frameFenceEvent);
        m_frameFenceEvent = nullptr;
    }
}

void SwapChain::InitializeSynchronization() {
    // Initialize fence values array - one per back buffer
    m_frameFenceValues.resize(m_bufferCount, 0);

    // Create the fence object
    HRESULT hr = m_device->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_frameFence));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create frame fence");
    }

    // Create fence event
    m_frameFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_frameFenceEvent) {
        throw std::runtime_error("Failed to create fence event");
    }

#ifdef _DEBUG
    m_frameFence->SetName(L"SwapChain Frame Fence");
#endif
}

void SwapChain::CreateSwapChain(UINT width, UINT height) {
    const DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;  // Constant format

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = m_bufferCount;
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = format;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    ComPtr<IDXGISwapChain1> swapChain1;
    HRESULT hr = m_device->GetFactory()->CreateSwapChainForHwnd(
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
    // Present the frame
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

    // Microsoft pattern: Signal fence after present
    const UINT64 currentFenceValue = m_nextFenceValue++;
    m_frameFenceValues[m_currentBackBufferIndex] = currentFenceValue;

    hr = m_commandQueue->Signal(m_frameFence.Get(), currentFenceValue);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to signal frame fence");
    }

    // Move to next frame
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Wait for the frame we're about to use to be ready
    WaitForFrame(m_currentBackBufferIndex);
}

void SwapChain::WaitForFrame(UINT frameIndex) {
    if (frameIndex >= m_frameFenceValues.size()) {
        return; // Invalid frame index
    }

    const UINT64 fenceValue = m_frameFenceValues[frameIndex];

    // If fence value is 0, this frame hasn't been used yet
    if (fenceValue == 0) {
        return;
    }

    // Check if frame is already complete
    if (m_frameFence->GetCompletedValue() >= fenceValue) {
        return; // Frame is already complete
    }

    // Wait for the frame to complete
    HRESULT hr = m_frameFence->SetEventOnCompletion(fenceValue, m_frameFenceEvent);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to set fence event");
    }

    WaitForSingleObject(m_frameFenceEvent, INFINITE);
}

void SwapChain::WaitForAllFrames() {
    // Wait for the latest fence value to complete
    if (m_nextFenceValue > 1) {
        const UINT64 lastFenceValue = m_nextFenceValue - 1;
        if (m_frameFence->GetCompletedValue() < lastFenceValue) {
            HRESULT hr = m_frameFence->SetEventOnCompletion(lastFenceValue, m_frameFenceEvent);
            if (SUCCEEDED(hr)) {
                WaitForSingleObject(m_frameFenceEvent, INFINITE);
            }
        }
    }
}

bool SwapChain::IsFrameComplete(UINT frameIndex) const {
    if (frameIndex >= m_frameFenceValues.size()) {
        return true; // Invalid frame index, consider complete
    }

    const UINT64 fenceValue = m_frameFenceValues[frameIndex];
    if (fenceValue == 0) {
        return true; // Frame hasn't been used yet
    }

    return m_frameFence->GetCompletedValue() >= fenceValue;
}

void SwapChain::Reconfigure(UINT width, UINT height, UINT bufferCount) {
    // Use current values if not specified
    UINT newBufferCount = (bufferCount == 0) ? m_bufferCount : bufferCount;

    // Validate buffer count
    if (newBufferCount < 2 || newBufferCount > 16) {
        throw std::invalid_argument("Back buffer count must be between 2 and 16");
    }

    // Invalid size check
    if (width == 0 || height == 0) {
        return; // Window minimized or invalid size
    }

    // Wait for gpu work before releasing buffers
    WaitForAllFrames();

    // Release ALL back buffer references
    ReleaseBackBuffers();

    // Resize the swap chain buffers
    HRESULT hr = m_swapChain->ResizeBuffers(
        newBufferCount,
        width,
        height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
    );

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to resize swap chain buffers. HRESULT: " + std::to_string(hr));
    }

    // Update properties
    m_bufferCount = newBufferCount;
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Reset fence values for new buffer configuration
    m_frameFenceValues.clear();
    m_frameFenceValues.resize(m_bufferCount, 0);

    // Recreate back buffer references
    CreateBackBuffers();
}

ID3D12Resource* SwapChain::GetCurrentBackBuffer() const {
    if (m_currentBackBufferIndex >= m_backBuffers.size()) {
        return nullptr;
    }
    return m_backBuffers[m_currentBackBufferIndex].Get();
}
