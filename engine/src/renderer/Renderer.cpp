#include "Renderer.h"

Renderer::Renderer(HWND hwnd) {
    m_device = std::make_unique<DX12Device>();
    if (!m_device->Initialize()) {
        throw std::runtime_error("Failed to initialize DirectX 12 device.");
    }
}
