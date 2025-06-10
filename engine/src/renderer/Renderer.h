#pragma once

#include "dx12/core/DX12Device.h"

class Renderer {
    std::unique_ptr<DX12Device> m_device;
    // DX12CommandQueue m_commandQueue;
    // DX12SwapChain m_swapChain;
    // DX12Resources m_resources;

public:
    Renderer(HWND hwnd);

    void BeginFrame();
    // void DrawMesh(Mesh& mesh, Material& material);
    void EndFrame();
    void Shutdown();
};
