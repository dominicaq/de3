#include "Renderer.h"

Renderer::Renderer(HWND hwnd, const EngineConfig& config) {
    m_device = std::make_unique<DX12Device>();
    if (!m_device->Initialize(true)) {
        throw std::runtime_error("Failed to initialize device");
    }

    // Create command queue manager
    m_commandManager = std::make_unique<CommandQueueManager>();
    if (!m_commandManager->Initialize(m_device->GetDevice())) {
        throw std::runtime_error("Failed to create command queues");
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

void Renderer::Render(const EngineConfig& config) {
    // Frame synchronization
    m_commandManager->GetGraphicsQueue()->Flush();

    // Command recording setup
    if (!m_commandAllocator->Reset()) {
        return;
    }
    if (!m_commandList->Reset(m_commandAllocator.get())) {
        return;
    }

    // TODO: Record commands here

    // Finalize and execute
    if (!m_commandList->Close()) {
        return;
    }

    ID3D12CommandList* commandLists[] = { m_commandList->GetCommandList() };
    m_commandManager->GetGraphicsQueue()->ExecuteCommandLists(1, commandLists);

    // Present and sync
    uint64_t fenceValue = m_commandManager->GetGraphicsQueue()->Signal();
    m_swapChain->Present(config.vsync);
    m_commandManager->GetGraphicsQueue()->WaitForFenceValue(fenceValue);
}

void Renderer::OnReconfigure(UINT width, UINT height, UINT bufferCount) {
    if (m_swapChain) {
        m_swapChain->Reconfigure(width, height, bufferCount);
    }
}
