#pragma once
#include "../Config.h"
#include "dx12/core/DX12Device.h"
#include "dx12/core/SwapChain.h"
// Command System
#include "dx12/core/CommandAllocator.h"
#include "dx12/core/CommandList.h"
#include "dx12/resources/FrameResources.h"
#include "dx12/CommandQueueManager.h"

class Renderer {
public:
    Renderer(HWND hwnd, const EngineConfig& config);
    ~Renderer();

    // Configuration & Lifecycle
    void OnReconfigure(UINT width, UINT height, UINT bufferCount = 0);

    // Frame Management
    CommandList* BeginFrame();
    void EndFrame(const EngineConfig& config);

    // Synchronization
    void WaitForFrame(UINT frameIndex);
    void WaitForAllFrames();
    bool IsFrameComplete(UINT frameIndex) const;
    void WaitForUploadCompletion();

    // Render Operations
    void SetupRenderTarget(CommandList* cmdList);
    void SetupViewportAndScissor(CommandList* cmdList);
    void ClearBackBuffer(CommandList* cmdList, const float clearColor[4]);

    // Property Accessors
    UINT GetBackBufferWidth() const;
    UINT GetBackBufferHeight() const;
    DXGI_FORMAT GetBackBufferFormat() const;

    // Geometry Management
    DX12Device* GetDevice() { return m_device.get(); }

    // Draw Call
    // void DrawMesh(CommandList* cmdList, MeshHandle meshHandle);

    // Debug/Development
    void DebugPrintValidationMessages() { m_device->PrintAndClearInfoQueue(); }

    // TODO: Testing/Development
    void TestMeshDraw(CommandList* cmdList);
    void FlushGPU();
private:
    // Initialization
    bool InitializeFrameResources();
    // void CreateTestMeshes();

    // Core DX12 Context
    std::unique_ptr<DX12Device> m_device;
    std::unique_ptr<CommandQueueManager> m_commandManager;
    std::unique_ptr<SwapChain> m_swapChain;

    // RTV Management
    bool CreateBackBufferRTVs();
    void ReleaseBackBufferRTVs();
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRTV(UINT index) const;
    ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
    UINT m_rtvDescriptorSize = 0;

    // Frame Resources & Synchronization
    std::vector<FrameResources> m_frameResources;
    UINT m_currentFrameIndex = 0;
    UINT64 m_nextFenceValue = 1;

    // Command Recording
    std::unique_ptr<CommandList> m_commandList;
};
