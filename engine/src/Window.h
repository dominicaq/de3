#pragma once
#include <windows.h>
#include <string>
#include <cstdint>

class Window {
public:
    Window() = default;
    ~Window();

    // Core window functions
    bool Create();
    void Destroy();
    void ProcessEvents();

    // Window state
    bool ShouldClose() const { return m_shouldClose; }
    void Close() { m_shouldClose = true; }

    // Getters
    HWND GetHandle() const { return m_hwnd; }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    bool IsFullscreen() const { return m_isFullscreen; }

    // Screen/display info
    uint32_t GetScreenWidth() const;
    uint32_t GetScreenHeight() const;
    void GetScreenDimensions(uint32_t& width, uint32_t& height) const;

    // Resolution management (goes through settings, not direct resize)
    bool ChangeResolution(uint32_t width, uint32_t height);
    bool SetFullscreen(bool fullscreen);

    // VSync management
    void SetVSync(bool enabled);

private:
    HWND m_hwnd = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    bool m_shouldClose = false;
    bool m_isFullscreen = false;

    // Store windowed mode position/size for fullscreen toggle
    RECT m_windowedRect = {};
    DWORD m_windowedStyle = 0;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Helper functions
    void UpdateWindowSize();
    bool RegisterWindowClass();
    void CenterWindow();
};
