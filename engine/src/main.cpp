#include <windows.h>
#include <iostream>
#include "Window.h"
#include "Config.h"

// Define the global config
EngineConfig g_config;

// TODO: Add your DirectX 12 initialization and rendering functions here
bool InitializeDirectX12(HWND hwnd) {
    // Initialize DirectX 12 device, command queue, swap chain, etc.
    // Use g_config.vsync for swap chain creation
    // Use g_config.windowWidth/windowHeight for render targets
    std::cout << "DirectX 12 initialization placeholder" << std::endl;
    return true;
}

void Render() {
    // Your DX12 rendering code here
    // Clear render targets, draw geometry, present, etc.
}

void Shutdown() {
    // Clean up DirectX 12 resources
    std::cout << "DirectX 12 shutdown placeholder" << std::endl;
}

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

    // Initialize DirectX 12
    if (!InitializeDirectX12(window.GetHandle())) {
        std::cerr << "Failed to initialize DirectX 12!" << std::endl;
        window.Destroy();
        return -1;
    }

    std::cout << "Engine initialized successfully!" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "  Alt+Enter - Toggle Fullscreen" << std::endl;

    // Main game loop
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
        Render();

        // Optional: Frame rate limiting
        if (!g_config.unlimitedFPS) {
            // Simple frame rate limiting (you might want a more sophisticated timer)
            Sleep(1000 / g_config.targetFPS);
        }
    }

    std::cout << "Shutting down engine..." << std::endl;

    // Clean shutdown
    Shutdown();
    window.Destroy();

    std::cout << "Engine shutdown complete." << std::endl;
    return 0;
}
