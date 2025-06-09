#include "Renderer.h"

Renderer::Renderer(HWND hwnd) {
    DX12Device device;
    if (!device.Initialize()) {
        // Handle failure
        return;
    }
}
