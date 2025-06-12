#pragma once
#include "../Config.h"
#include "dx12/core/DX12Device.h"
#include "dx12/core/SwapChain.h"

// Command System
#include "dx12/core/CommandAllocator.h"
#include "dx12/core/CommandList.h"
#include "dx12/CommandQueueManager.h"

class Renderer {
public:
    Renderer(HWND hwnd, const EngineConfig& config);
    void Render(const EngineConfig& config);
    void OnReconfigure(UINT width, UINT height, UINT bufferCount = 0);

private:
    std::unique_ptr<DX12Device> m_device;
    std::unique_ptr<CommandQueueManager> m_commandManager;
    std::unique_ptr<SwapChain> m_swapChain;

    // Command recording objects
    std::unique_ptr<CommandAllocator> m_commandAllocator;
    std::unique_ptr<CommandList> m_commandList;
};
