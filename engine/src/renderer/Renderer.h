#pragma once
#include "../Config.h"
#include "dx12/core/DX12Device.h"
#include "dx12/core/SwapChain.h"
// Command System
#include "dx12/core/CommandAllocator.h"
#include "dx12/core/CommandList.h"
#include "dx12/resources/FrameResources.h"
#include "dx12/CommandQueueManager.h"
// Geometry System
#include "../resources/GeometryManager.h"
// TEMP
#include "dx12/resources/Shader.h"

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
    GeometryManager* GetGeometryManager() const { return m_geometryManager.get(); }

    // Debug/Development
    void DebugPrintValidationMessages() { m_device->PrintAndClearInfoQueue(); }

    // Testing/Development - Clean Interface
    void TestMeshDraw(CommandList* cmdList);

private:
    // Initialization
    bool InitializeFrameResources();
    void CreateTestMeshes();

    // Core DX12 Context
    std::unique_ptr<DX12Device> m_device;
    std::unique_ptr<CommandQueueManager> m_commandManager;
    std::unique_ptr<SwapChain> m_swapChain;

    // Geometry System
    std::unique_ptr<GeometryManager> m_geometryManager;
    MeshHandle m_triangleMesh = INVALID_MESH_HANDLE;

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

    // Testing - Shader System
    std::unique_ptr<Shader> m_testShader;
};
