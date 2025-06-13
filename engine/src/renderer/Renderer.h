#pragma once
#include "../Config.h"
#include "dx12/core/DX12Device.h"
#include "dx12/core/SwapChain.h"

// Command System
#include "dx12/core/CommandAllocator.h"
#include "dx12/core/CommandList.h"
#include "dx12/CommandQueueManager.h"
#include "dx12/ResourceManager.h"

class Renderer {
public:
    Renderer(HWND hwnd, const EngineConfig& config);
    void OnReconfigure(UINT width, UINT height, UINT bufferCount = 0);
    CommandList* BeginFrame();
    void EndFrame(const EngineConfig& config);
    void ClearBackBuffer(CommandList* cmdList, float clearColor[4]);

private:
    // DX12 Context
    std::unique_ptr<DX12Device> m_device;
    std::unique_ptr<CommandQueueManager> m_commandManager;
    std::unique_ptr<SwapChain> m_swapChain;

    // Command recording objects
    std::unique_ptr<CommandAllocator> m_commandAllocator;
    std::unique_ptr<CommandList> m_commandList;

    // Resources
    std::unique_ptr<ResourceManager> m_resourceManager;
};
