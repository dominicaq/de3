#pragma once

#include "DX12Common.h"
#include <vector>

// Forward declarations
struct ID3D12Resource;
struct ID3D12CommandQueue;
struct IDXGIFactory4;
struct EngineConfig;

class SwapChain {
public:
    SwapChain(IDXGIFactory4* factory, ID3D12CommandQueue* commandQueue, HWND hwnd, const EngineConfig& config);
    ~SwapChain() = default;

    // Non-copyable
    SwapChain(const SwapChain&) = delete;
    SwapChain& operator=(const SwapChain&) = delete;

    void Present(bool vsync);
    void Resize(UINT width, UINT height);

    // Getters
    ID3D12Resource* GetCurrentBackBuffer() const;

private:
    void CreateSwapChain(IDXGIFactory4* factory);
    void CreateBackBuffers();
    void ReleaseBackBuffers();

    ID3D12CommandQueue* m_commandQueue;
    HWND m_hwnd;
    const EngineConfig& m_config;

    ComPtr<IDXGISwapChain3> m_swapChain;
    std::vector<ComPtr<ID3D12Resource>> m_backBuffers;

    UINT m_width;
    UINT m_height;
    DXGI_FORMAT m_format;
    UINT m_currentBackBufferIndex;
    UINT m_bufferCount;
};
