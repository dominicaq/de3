#pragma once
#include <windows.h>
#include <functional>
#include "Config.h"

class Window {
public:
    using ResizeCallback = std::function<void(UINT width, UINT height)>;

    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool Create(const EngineConfig& config);
    void Destroy();
    void ProcessEvents();
    void Close() { m_shouldClose = true; }

    bool ChangeResolution(UINT width, UINT height);
    bool SetFullscreen(bool fullscreen);

    void SetResizeCallback(ResizeCallback callback) { m_resizeCallback = callback; }

    HWND GetHandle() const { return m_hwnd; }
    bool ShouldClose() const { return m_shouldClose; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    bool RegisterWindowClass();
    void UpdateWindowSize();
    void CenterWindow();

    HWND m_hwnd = nullptr;
    bool m_shouldClose = false;
    DWORD m_windowedStyle = 0;
    RECT m_windowedRect = {};
    const EngineConfig* m_config = nullptr;
    ResizeCallback m_resizeCallback;
};
