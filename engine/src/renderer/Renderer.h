#pragma once

class Renderer {
    // DX12Device m_device;
    // DX12CommandQueue m_commandQueue;
    // DX12SwapChain m_swapChain;
    // DX12Resources m_resources;

public:
    // bool Initialize(HWND hwnd);
    void BeginFrame();
    // void DrawMesh(Mesh& mesh, Material& material);
    void EndFrame();
    void Shutdown();
};
