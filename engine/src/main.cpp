#include <windows.h>
#include <iostream>
#include "Window.h"
#include "Config.h"
// Engine includes
#include "renderer/Renderer.h"

EngineConfig g_config;

int main() {
    std::cout << "Starting game engine..." << std::endl;
    std::cout << "Resolution: " << g_config.windowWidth << "x" << g_config.windowHeight << std::endl;
    std::cout << "VSync: " << (g_config.vsync ? "Enabled" : "Disabled") << std::endl;
    std::cout << "Fullscreen: " << (g_config.fullscreen ? "Yes" : "No") << std::endl;

    // Create window
    Window window;
    if (!window.Create()) {
        std::cerr << "Failed to create window!" << std::endl;
        return -1;
    }

    std::unique_ptr<Renderer> renderer;

    try {
        // Initialize DirectX 12 - only catch renderer setup errors
        renderer = std::make_unique<Renderer>(window.GetHandle());
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Renderer Setup Error: " << e.what() << std::endl;
        MessageBoxA(window.GetHandle(), e.what(), "Graphics Error", MB_OK | MB_ICONERROR);
        window.Destroy();
        return -1;
    }

    std::cout << "Engine initialized successfully!" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "  Alt+Enter - Toggle Fullscreen" << std::endl;

    // Main game loop - no exception handling, let engine errors crash naturally
    while (!window.ShouldClose()) {
        // Process Windows messages and input
        window.ProcessEvents();

        // Example: Toggle VSync with F1 key
        if (GetAsyncKeyState(VK_F1) & 0x8000) {
            static bool f1Pressed = false;
            if (!f1Pressed) {
                window.SetVSync(!g_config.vsync);
                f1Pressed = true;
            }
        } else {
            static bool f1Pressed = false;
            f1Pressed = false;
        }

        // Example: Change resolution with number keys (for testing)
        if (GetAsyncKeyState('1') & 0x8000) {
            static bool key1Pressed = false;
            if (!key1Pressed) {
                window.ChangeResolution(1280, 720);
                key1Pressed = true;
            }
        } else {
            static bool key1Pressed = false;
            key1Pressed = false;
        }

        // Render frame

        // Optional: Frame rate limiting
        if (!g_config.unlimitedFPS) {
            Sleep(1000 / g_config.targetFPS);
        }
    }

    std::cout << "Shutting down engine..." << std::endl;
    window.Destroy();
    std::cout << "Engine shutdown complete." << std::endl;
    return 0;
}
