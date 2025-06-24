#pragma once
#include "DX12Common.h"
#include "DX12Device.h"
#include <vector>

struct ID3D12Resource;
struct ID3D12CommandQueue;

class SwapChain {
public:
    SwapChain(DX12Device* device, ID3D12CommandQueue* commandQueue, HWND hwnd,
              UINT width, UINT height, UINT bufferCount, DXGI_FORMAT bufferFormat);
    ~SwapChain();
    SwapChain(const SwapChain&) = delete;
    SwapChain& operator=(const SwapChain&) = delete;

    // Core functionality
    bool Present(bool vsync);
    bool Reconfigure(UINT width, UINT height, UINT bufferCount = 0); // 0 = Keep current count

    // Status
    bool IsInitialized() const { return m_initialized; }

    // Buffer access - pure resource access, no views
    UINT GetCurrentBackBufferIndex() const { return m_currentBackBufferIndex; }
    UINT GetBufferCount() const { return m_bufferCount; }
    ID3D12Resource* GetCurrentBackBuffer() const { return m_backBuffers[m_currentBackBufferIndex].Get(); }
    ID3D12Resource* GetBackBuffer(UINT index) const { return (index < m_bufferCount) ? m_backBuffers[index].Get() : nullptr; }

    // Getters
    UINT GetWidth() const { return m_width; }
    UINT GetHeight() const { return m_height; }
    DXGI_FORMAT GetFormat() const { return m_bufferFormat; }

private:
    bool Initialize(UINT width, UINT height);
    bool CreateSwapChain(UINT width, UINT height);
    bool CreateBackBuffers();
    void ReleaseBackBuffers();

    DX12Device* m_device;
    ID3D12CommandQueue* m_commandQueue;
    HWND m_hwnd;
    DXGI_FORMAT m_bufferFormat;

    ComPtr<IDXGISwapChain3> m_swapChain;
    std::vector<ComPtr<ID3D12Resource>> m_backBuffers;

    UINT m_width;
    UINT m_height;
    UINT m_currentBackBufferIndex;
    UINT m_bufferCount;
    bool m_initialized = false;
};
