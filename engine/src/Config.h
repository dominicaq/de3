#pragma once

#include <string>
#include <cstdint>
#include <iostream>

struct EngineConfig {
    // Window settings
    uint32_t windowWidth = 800;
    uint32_t windowHeight = 600;
    std::string windowTitle = "DirectX12 Game";
    bool fullscreen = false;
    bool vsync = false;

    // Renderer settings
    bool enableDebugLayer = false;
    uint32_t maxFramesInFlight = 2;

    // Performance settings
    uint32_t targetFPS = 180;
    bool cappedFPS = true;
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
    std::cout << "Max Frames in Flight: " << config.maxFramesInFlight << std::endl;

    // Performance Settings
    std::cout << "\n[Performance Settings]" << std::endl;
    std::cout << "Target FPS: " << config.targetFPS << std::endl;
    std::cout << "Capped FPS: " << (config.cappedFPS ? "Enabled" : "Disabled") << std::endl;

    std::cout << "================================" << std::endl;
}
