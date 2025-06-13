#include "Renderer.h"

Renderer::Renderer(HWND hwnd, const EngineConfig& config) {
    m_device = std::make_unique<DX12Device>();
    if (!m_device->Initialize(config.enableDebugLayer)) {
        throw std::runtime_error("Failed to initialize device");
    }

    // Create command queue manager
    m_commandManager = std::make_unique<CommandQueueManager>();
    if (!m_commandManager->Initialize(m_device->GetDevice())) {
        throw std::runtime_error("Failed to create command queues");
    }

    // Create resource manager
    m_resourceManager = std::make_unique<ResourceManager>();
    if (!m_resourceManager->Initialize(m_device.get(), m_commandManager->GetGraphicsQueue()->GetCommandQueue())) {
        throw std::runtime_error("Failed to initialize resource manager");
    }

    // Create command recording objects
    m_commandAllocator = std::make_unique<CommandAllocator>();
    if (!m_commandAllocator->Initialize(m_device->GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT)) {
        throw std::runtime_error("Failed to create command allocator");
    }

    // Create command list with the allocator
    m_commandList = std::make_unique<CommandList>();
    if (!m_commandList->Initialize(m_device->GetDevice(), m_commandAllocator.get(), D3D12_COMMAND_LIST_TYPE_DIRECT)) {
        throw std::runtime_error("Failed to create command list");
    }

    // Create SwapChain - use graphics queue
    m_swapChain = std::make_unique<SwapChain>(
        m_device.get(),
        m_commandManager->GetGraphicsQueue()->GetCommandQueue(),
        hwnd,
        config.windowWidth,
        config.windowHeight,
        config.backBufferCount
    );

    if (!m_swapChain) {
        throw std::runtime_error("Failed to create swap chain");
    }
}

CommandList* Renderer::BeginFrame() {
    m_resourceManager->BeginFrame();
    m_commandManager->GetGraphicsQueue()->Flush();

    if (!m_commandAllocator->Reset()) {
        return nullptr;
    }
    if (!m_commandList->Reset(m_commandAllocator.get())) {
        return nullptr;
    }

    // Transition back buffer to render target
    ID3D12Resource* backBuffer = m_swapChain->GetCurrentBackBuffer();
    m_commandList->TransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    return m_commandList.get();
}

void Renderer::EndFrame(const EngineConfig& config) {
    // Transition back buffer to present
    ID3D12Resource* backBuffer = m_swapChain->GetCurrentBackBuffer();
    m_commandList->TransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    if (!m_commandList->Close()) {
        return;
    }

    ID3D12CommandList* commandLists[] = { m_commandList->GetCommandList() };
    m_commandManager->GetGraphicsQueue()->ExecuteCommandLists(1, commandLists);

    uint64_t fenceValue = m_commandManager->GetGraphicsQueue()->Signal();
    m_resourceManager->EndFrame();
    m_swapChain->Present(config.vsync);
    m_commandManager->GetGraphicsQueue()->WaitForFenceValue(fenceValue);
}

void Renderer::ClearBackBuffer(CommandList* cmdList, float clearColor[4]) {
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swapChain->GetCurrentBackBufferRTV();
    cmdList->ClearRenderTarget(rtv, clearColor);
}

void Renderer::OnReconfigure(UINT width, UINT height, UINT bufferCount) {
    if (m_swapChain) {
        m_swapChain->Reconfigure(width, height, bufferCount);
    }
}
