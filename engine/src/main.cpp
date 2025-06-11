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
        // Initialize DirectX 12 - only catch renderer setup errors
        renderer = std::make_unique<Renderer>(window.GetHandle(), g_config);
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Renderer Setup Error: " << e.what() << std::endl;
        MessageBoxA(window.GetHandle(), e.what(), "Graphics Error", MB_OK | MB_ICONERROR);
        window.Destroy();
        return -1;
    }

    PrintConfigStats(g_config);

    // Game loop
    while (!window.ShouldClose()) {
        // Process Windows messages and input
        window.ProcessEvents();

        // Render frame
        renderer->Render(g_config);
        if (g_config.cappedFPS && !g_config.vsync) {
            FPSUtils::LimitFrameRate(g_config.targetFPS);
        }

        // FPS Logger
        float fps;
        if (FPSUtils::UpdateFPSCounter(fps, 1000)) { // Update every second
            std::cout << "FPS: " << fps << std::endl;
        }
    }

    std::cout << "Shutting down engine..." << std::endl;
    window.Destroy();
    std::cout << "Engine shutdown complete." << std::endl;
    return 0;
}
