#pragma once
#include "DX12Common.h"
#include "DX12Device.h"
#include <vector>

struct ID3D12Resource;
struct ID3D12CommandQueue;

class SwapChain {
public:
    SwapChain(DX12Device* device, ID3D12CommandQueue* commandQueue, HWND hwnd,
              UINT width, UINT height, UINT bufferCount);
    ~SwapChain();

    SwapChain(const SwapChain&) = delete;
    SwapChain& operator=(const SwapChain&) = delete;

    bool Present(bool vsync);
    bool Reconfigure(UINT width, UINT height, UINT bufferCount = 0); // 0 = Keep current count

    void WaitForFrame(UINT frameIndex);
    void WaitForAllFrames();
    bool IsFrameComplete(UINT frameIndex) const;

    bool IsInitialized() const { return m_initialized; }

    ID3D12Resource* GetCurrentBackBuffer() const { return m_backBuffers[m_currentBackBufferIndex].Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV() const;

private:
    bool Initialize(UINT width, UINT height);
    bool CreateSwapChain(UINT width, UINT height);
    bool CreateBackBuffers();
    bool CreateRTVs();
    void ReleaseBackBuffers();
    void ReleaseRTVs();
    bool InitializeSynchronization();

    DX12Device* m_device;
    ID3D12CommandQueue* m_commandQueue;
    HWND m_hwnd;
    ComPtr<IDXGISwapChain3> m_swapChain;
    std::vector<ComPtr<ID3D12Resource>> m_backBuffers;

    ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
    UINT m_rtvDescriptorSize;

    std::vector<UINT64> m_frameFenceValues;
    ComPtr<ID3D12Fence> m_frameFence;
    HANDLE m_frameFenceEvent;
    UINT64 m_nextFenceValue = 1;

    UINT m_currentBackBufferIndex;
    UINT m_bufferCount;
    bool m_initialized = false;
};
