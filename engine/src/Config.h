#pragma once
#include <string>
#include <cstdint>

struct EngineConfig {
    // Window settings
    uint32_t windowWidth = 800;
    uint32_t windowHeight = 600;
    std::string windowTitle = "DirectX12 Game";
    bool fullscreen = false;
    bool vsync = true;

    // Renderer settings
    bool enableDebugLayer = false;
    uint32_t maxFramesInFlight = 2;

    // Performance settings
    uint32_t targetFPS = 60;
    bool unlimitedFPS = false;
};

// Global config
extern EngineConfig g_config;
