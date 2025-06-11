#pragma once

#include "../Config.h"
#include "dx12/core/DX12Device.h"
#include "dx12/core/DX12SwapChain.h"

class Renderer {
public:
    Renderer(HWND hwnd, const EngineConfig& config);
    void Render(const EngineConfig& config);
    void OnResize(UINT width, UINT height);

private:
    std::unique_ptr<DX12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    std::unique_ptr<SwapChain> m_swapChain;
};
