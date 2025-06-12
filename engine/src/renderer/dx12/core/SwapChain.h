#pragma once
#include "DX12Common.h"
#include "DX12Device.h"
#include <vector>

// Forward declarations
struct ID3D12Resource;
struct ID3D12CommandQueue;

class SwapChain {
public:
    SwapChain(DX12Device* device, ID3D12CommandQueue* commandQueue, HWND hwnd,
              UINT width, UINT height, UINT bufferCount);
    ~SwapChain();

    // Non-copyable
    SwapChain(const SwapChain&) = delete;
    SwapChain& operator=(const SwapChain&) = delete;

    // Presentation and reconfiguration
    bool Present(bool vsync);
    bool Reconfigure(UINT width, UINT height, UINT bufferCount = 0);  // 0 = keep current count

    // Synchronization methods
    void WaitForFrame(UINT frameIndex);
    void WaitForAllFrames();
    bool IsFrameComplete(UINT frameIndex) const;

    // State checking
    bool IsInitialized() const { return m_initialized; }

    // Getters
    ID3D12Resource* GetCurrentBackBuffer() const;
    UINT GetCurrentBackBufferIndex() const { return m_currentBackBufferIndex; }
    UINT GetBufferCount() const { return m_bufferCount; }

private:
    // Initialization methods
    bool Initialize(UINT width, UINT height);
    bool CreateSwapChain(UINT width, UINT height);
    bool CreateBackBuffers();
    void ReleaseBackBuffers();
    bool InitializeSynchronization();

    // Core objects
    DX12Device* m_device;
    ID3D12CommandQueue* m_commandQueue;
    HWND m_hwnd;
    ComPtr<IDXGISwapChain3> m_swapChain;
    std::vector<ComPtr<ID3D12Resource>> m_backBuffers;

    // Synchronization
    std::vector<UINT64> m_frameFenceValues;
    ComPtr<ID3D12Fence> m_frameFence;
    HANDLE m_frameFenceEvent;
    UINT64 m_nextFenceValue = 1;

    // State
    UINT m_currentBackBufferIndex;
    UINT m_bufferCount;
    bool m_initialized = false;
};
