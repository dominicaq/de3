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

    bool IsInitialized() const { return m_initialized; }
    UINT GetCurrentBackBufferIndex() const { return m_currentBackBufferIndex; }
    UINT GetBufferCount() const { return m_bufferCount; }

    ID3D12Resource* GetCurrentBackBuffer() const { return m_backBuffers[m_currentBackBufferIndex].Get(); }
    ID3D12Resource* GetBackBuffer(UINT index) const { return (index < m_bufferCount) ? m_backBuffers[index].Get() : nullptr; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRTV(UINT index) const;

    // Getters
    UINT GetWidth() const { return m_width; }
    UINT GetHeight() const { return m_height; }

private:
    bool Initialize(UINT width, UINT height);
    bool CreateSwapChain(UINT width, UINT height);
    bool CreateBackBuffers();
    bool CreateRTVs();
    void ReleaseBackBuffers();

    DX12Device* m_device;
    ID3D12CommandQueue* m_commandQueue;
    HWND m_hwnd;

    ComPtr<IDXGISwapChain3> m_swapChain;
    std::vector<ComPtr<ID3D12Resource>> m_backBuffers;

    ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
    UINT m_rtvDescriptorSize;

    UINT m_width;
    UINT m_height;

    UINT m_currentBackBufferIndex;
    UINT m_bufferCount;
    bool m_initialized = false;
};
