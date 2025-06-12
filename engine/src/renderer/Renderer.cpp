#include "Renderer.h"

Renderer::Renderer(HWND hwnd, const EngineConfig& config) {
    m_device = std::make_unique<DX12Device>();
    if (!m_device->Initialize()) {
        throw std::runtime_error("Failed to initialize device");
    }

    // Create command queue (TEMP)
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    HRESULT hr = m_device->GetDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create command queue");
    }

    // Create SwapChain - extract values from config
    m_swapChain = std::make_unique<SwapChain>(
        m_device.get(),
        m_commandQueue.Get(),
        hwnd,
        config.windowWidth,
        config.windowHeight,
        config.backBufferCount
    );
}

void Renderer::Render(const EngineConfig& config) {
    // Record commands to m_swapChain->GetCurrentBackBuffer()
    // ...
    m_swapChain->Present(config.vsync);
}

void Renderer::OnReconfigure(UINT width, UINT height, UINT bufferCount) {
    if (m_swapChain) {
        m_swapChain->Reconfigure(width, height, bufferCount);
    }
}
