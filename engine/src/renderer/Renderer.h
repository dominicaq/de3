#pragma once
#include "../Config.h"
#include "dx12/core/DX12Device.h"
#include "dx12/core/SwapChain.h"
#include "dx12/core/CommandAllocator.h"
#include "dx12/core/CommandList.h"
#include "dx12/resources/FrameResources.h"
#include "dx12/CommandQueueManager.h"

class Renderer {
public:
    Renderer(HWND hwnd, const EngineConfig& config);
    ~Renderer();

    void OnReconfigure(UINT width, UINT height, UINT bufferCount = 0);

    CommandList* BeginFrame();
    void EndFrame(const EngineConfig& config);

    void WaitForFrame(UINT frameIndex);
    void WaitForAllFrames();
    bool IsFrameComplete(UINT frameIndex) const;
    void WaitForUploadCompletion();

    void SetupRenderTarget(CommandList* cmdList);
    void SetupViewportAndScissor(CommandList* cmdList);
    void ClearBackBuffer(CommandList* cmdList, const float clearColor[4]);
    void ClearDepthBuffer(CommandList* cmdList, float depth = 1.0f, UINT8 stencil = 0);

    UINT GetBackBufferWidth() const;
    UINT GetBackBufferHeight() const;
    DXGI_FORMAT GetBackBufferFormat() const;

    DX12Device* GetDevice() { return m_device.get(); }

    void DebugPrintValidationMessages() { m_device->PrintAndClearInfoQueue(); }
    void FlushGPU();

private:
    bool InitializeFrameResources();
    bool CreateDepthBuffer();
    void ReleaseDepthBuffer();

    std::unique_ptr<DX12Device> m_device;
    std::unique_ptr<CommandQueueManager> m_commandManager;
    std::unique_ptr<SwapChain> m_swapChain;

    bool CreateBackBufferRTVs();
    void ReleaseBackBufferRTVs();
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRTV(UINT index) const;
    ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
    UINT m_rtvDescriptorSize = 0;

    ComPtr<ID3D12Resource> m_depthStencilBuffer;
    ComPtr<ID3D12DescriptorHeap> m_dsvDescriptorHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_depthStencilView;

    std::vector<FrameResources> m_frameResources;
    UINT m_currentFrameIndex = 0;
    UINT64 m_nextFenceValue = 1;

    std::unique_ptr<CommandList> m_commandList;
};
