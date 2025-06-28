#pragma once
#include "../Config.h"
#include "dx12/core/DX12Device.h"
#include "dx12/core/SwapChain.h"
// Command System
#include "dx12/core/CommandAllocator.h"
#include "dx12/core/CommandList.h"
#include "dx12/resources/FrameResources.h"
#include "dx12/CommandQueueManager.h"

// TEMP
#include "dx12/resources/Shader.h"
#include "../resources/GeometryManager.h"

class Renderer {
public:
    Renderer(HWND hwnd, const EngineConfig& config);
    ~Renderer();

    // Configuration
    void OnReconfigure(UINT width, UINT height, UINT bufferCount = 0);

    // Frame management
    CommandList* BeginFrame();
    void EndFrame(const EngineConfig& config);

    // Render target operations - for render passes to use
    void SetupRenderTarget(CommandList* cmdList);
    void SetupViewportAndScissor(CommandList* cmdList);
    void ClearBackBuffer(CommandList* cmdList, const float clearColor[4]);

    // Synchronization
    void WaitForFrame(UINT frameIndex);
    void WaitForAllFrames();
    bool IsFrameComplete(UINT frameIndex) const;

    // Accessors for render passes
    UINT GetBackBufferWidth() const;
    UINT GetBackBufferHeight() const;
    DXGI_FORMAT GetBackBufferFormat() const;

    // Debugging
    void DebugPrintValidationMessages() { m_device->PrintAndClearInfoQueue(); }

    // TODO: TEMP
    std::unique_ptr<GeometryManager> m_geometryManager;
    GeometryHandle m_triangleGeometry = INVALID_GEOMETRY_HANDLE;
    std::unique_ptr<Shader> m_testShader;
    void TestShaderDraw(CommandList* cmdList);
    void TEMP_FUNC();
    // END OF TEMP

private:
    bool InitializeFrameResources();

    // DX12 Context
    std::unique_ptr<DX12Device> m_device;
    std::unique_ptr<CommandQueueManager> m_commandManager;
    std::unique_ptr<SwapChain> m_swapChain;

    // TODO: Make this its own manager later
    // RTV Management
    bool CreateBackBufferRTVs();
    void ReleaseBackBufferRTVs();
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRTV(UINT index) const;

    ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
    UINT m_rtvDescriptorSize = 0;

    // Per-frame resources
    std::vector<FrameResources> m_frameResources;
    UINT m_currentFrameIndex = 0;
    UINT64 m_nextFenceValue = 1;

    // Shared command list (reset per frame with different allocators)
    std::unique_ptr<CommandList> m_commandList;
};
