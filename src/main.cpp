#include "Window.h"
#include <iostream>

int main() {
    // Create a DirectX 12 ready window
    Window window("DirectX 12 Window", 1920, 1080);

    // Create and show the window
    if (!window.Create()) {
        std::cerr << "Failed to create window!" << std::endl;
        return -1;
    }

    // Enable DirectX 12 specific features
    window.EnableDirectX12();
    window.Show();

    std::cout << "DirectX 12 window created successfully!" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  ESC - Close window" << std::endl;
    std::cout << "  F11 - Toggle fullscreen" << std::endl;
    std::cout << "  F1  - Toggle VSync" << std::endl;

    // Main message loop
    while (window.PollEvents()) {
        // Your DirectX 12 rendering would go here
        // The window is now ready for DX12 swap chain creation

        Sleep(16); // ~60 FPS
    }

    std::cout << "Window closed. Exiting..." << std::endl;
    return 0;
}
