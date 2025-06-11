#pragma once

#include <windows.h>
#include <cstdint>
#include "Config.h"

class Window {
public:
    Window() = default;
    ~Window();

    bool Create(const EngineConfig& config);
    void Destroy();
    void ProcessEvents();

    // Getters
    HWND GetHandle() const { return m_hwnd; }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    bool ShouldClose() const { return m_shouldClose; }
    bool IsFullscreen() const { return m_isFullscreen; }

    // Window operations - now require config parameter
    bool ChangeResolution(uint32_t width, uint32_t height, EngineConfig& config);
    bool SetFullscreen(bool fullscreen, EngineConfig& config);

    // Utility functions
    void GetScreenDimensions(uint32_t& width, uint32_t& height) const;
    void Close() { m_shouldClose = true; }

private:
    HWND m_hwnd = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    bool m_shouldClose = false;
    bool m_isFullscreen = false;

    DWORD m_windowedStyle = 0;
    RECT m_windowedRect = {};

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool RegisterWindowClass();
    void UpdateWindowSize();
    void CenterWindow();
};
