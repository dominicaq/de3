#pragma once

#include <string>
#include <cstdint>
#include <iostream>

#include <dxgi1_6.h>  // For DXGI_FORMAT

struct EngineConfig {
    // Window settings
    uint32_t windowWidth = 800;
    uint32_t windowHeight = 600;
    std::string windowTitle = "DirectX12 Game";
    bool fullscreen = false;
    bool vsync = true;

    // Renderer settings
    bool useTripleBuffering = true;
    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    bool cappedFPS = true;
    uint32_t targetFPS = 180;

    // DEBUG SETTINGS
    uint32_t debugFrameInterval = 60;
    bool enableDebugLayer = _DEBUG;

    uint32_t GetBufferCount() const {
        // Triple buffering only provides benefits with VSync enabled
        return (vsync && useTripleBuffering) ? 3 : 2;
    }
};

inline void PrintConfigStats(const EngineConfig& config) {
    std::cout << "=== Engine Configuration ===" << std::endl;

    // Window Settings
    std::cout << "\n[Window Settings]" << std::endl;
    std::cout << "Resolution: " << config.windowWidth << "x" << config.windowHeight << std::endl;
    std::cout << "Title: " << config.windowTitle << std::endl;
    std::cout << "Fullscreen: " << (config.fullscreen ? "Yes" : "No") << std::endl;
    std::cout << "VSync: " << (config.vsync ? "Enabled" : "Disabled") << std::endl;

    // Renderer Settings
    std::cout << "\n[Renderer Settings]" << std::endl;
    std::cout << "Debug Layer: " << (config.enableDebugLayer ? "Enabled" : "Disabled") << std::endl;
    std::cout << "Buffering: ";
    switch(config.GetBufferCount()) {
        case 2:
            std::cout << "Double buffering (2 buffers)" << std::endl;
            break;
        case 3:
            std::cout << "Triple buffering (3 buffers)" << std::endl;
            break;
        default:
            std::cout << config.GetBufferCount() << " buffers" << std::endl;
            break;
    }

    // Performance Settings
    std::cout << "\n[Performance Settings]" << std::endl;
    std::cout << "Target FPS: " << config.targetFPS << std::endl;
    std::cout << "Capped FPS: " << (config.cappedFPS ? "Enabled" : "Disabled") << std::endl;

    std::cout << "================================" << std::endl;
}
