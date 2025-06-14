#pragma once
#include "../Config.h"
#include "dx12/core/DX12Device.h"
#include "dx12/core/SwapChain.h"

// Command System
#include "dx12/core/CommandAllocator.h"
#include "dx12/core/CommandList.h"
#include "dx12/CommandQueueManager.h"
#include "dx12/ResourceManager.h"

#include "FrameResources.h"

class Renderer {
public:
    Renderer(HWND hwnd, const EngineConfig& config);
    ~Renderer();

    void OnReconfigure(UINT width, UINT height, UINT bufferCount = 0);

    CommandList* BeginFrame();
    void EndFrame(const EngineConfig& config);
    void ClearBackBuffer(CommandList* cmdList, float clearColor[4]);

    void WaitForFrame(UINT frameIndex);
    void WaitForAllFrames();
    bool IsFrameComplete(UINT frameIndex) const;

private:
    bool InitializeFrameResources();
    void ReleaseFrameResources();

    // DX12 Context
    std::unique_ptr<DX12Device> m_device;
    std::unique_ptr<CommandQueueManager> m_commandManager;
    std::unique_ptr<SwapChain> m_swapChain;

    // Per-frame resources
    std::vector<FrameResources> m_frameResources;
    UINT m_currentFrameIndex = 0;
    UINT64 m_nextFenceValue = 1;

    // Shared command list (reset per frame with different allocators)
    std::unique_ptr<CommandList> m_commandList;

    // Resources
    std::unique_ptr<ResourceManager> m_resourceManager;
};
