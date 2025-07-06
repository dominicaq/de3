#include <windows.h>
#include <iostream>
#include "Window.h"
#include "Config.h"
// Engine includes
#include "renderer/Renderer.h"
#include "renderer/FPSUtils.h"

static EngineConfig g_config;

int main() {
    Window window;
    if (!window.Create(g_config)) {
        std::cerr << "Failed to create window!" << std::endl;
        return -1;
    }

    std::unique_ptr<Renderer> renderer;
    try {
        // Initialize DirectX 12
        renderer = std::make_unique<Renderer>(window.GetHandle(), g_config);
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Graphics initialization failed: " << e.what() << std::endl;
        MessageBoxA(window.GetHandle(), e.what(), "DirectX 12 Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // Setup resize callback - SwapChain will handle GPU synchronization
    window.SetResizeCallback([&renderer](UINT width, UINT height) {
        if (renderer) {
            renderer->OnReconfigure(width, height);
        }
    });

    PrintConfigStats(g_config);

    // Game loop
    int frameCount = 0;
    while (!window.ShouldClose()) {
        window.ProcessEvents();
        // Begin frame and get command list
        CommandList* cmdList = renderer->BeginFrame();
        renderer->SetupRenderTarget(cmdList);
        renderer->SetupViewportAndScissor(cmdList);

        // Rainbow color that cycles over time
        static float time = 0.0f;
        time += 0.016f; // ~60fps increment

        float r = (sin(time * 2.0f) + 1.0f) * 0.5f;
        float g = (sin(time * 2.0f + 2.094f) + 1.0f) * 0.5f; // 2π/3 offset
        float b = (sin(time * 2.0f + 4.188f) + 1.0f) * 0.5f; // 4π/3 offset
        float clearColor[4] = { r, g, b, 1.0f };

        // Clear back buffer with animated rainbow color
        renderer->ClearBackBuffer(cmdList, clearColor);

        // Draw test triangle
        renderer->TestMeshDraw(cmdList);

        // static uint32_t frameCount = 0;
        // if (++frameCount % 180 * 9 == 0) {  // Every 9 seconds
        //     renderer->GetGeometryManager()->PrintDebugInfo();
        // }

        // Finish frame and present
        renderer->EndFrame(g_config);

#ifdef _DEBUG
        if (frameCount % g_config.targetFPS == 0) {
            renderer->DebugPrintValidationMessages();
        }
#endif

        if (g_config.cappedFPS && !g_config.vsync) {
            FPSUtils::LimitFrameRate(g_config.targetFPS);
        }

        float fps;
        if (FPSUtils::UpdateFPSCounter(fps, 1000)) {
            std::cout << "FPS: " << fps << std::endl;
        }
    }

    std::cout << "Shutting down engine..." << std::endl;
    window.Destroy();
    std::cout << "Engine shutdown complete." << std::endl;
    return 0;
}
