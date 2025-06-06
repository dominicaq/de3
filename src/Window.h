#pragma once
#include <windows.h>
#include <dwmapi.h>
#include <string>

class Window {
private:
    HWND m_hwnd;
    HINSTANCE m_hinstance;
    HDC m_hdc;
    std::string m_className;
    std::string m_windowTitle;
    int m_width;
    int m_height;
    bool m_isRunning;
    bool m_isFullscreen;
    bool m_vsyncEnabled;

    // Static window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Instance window procedure
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Internal helper methods
    void RegisterWindowClass();
    void CreateWindowHandle();
    void SetupForDirectX();

public:
    Window(const std::string& title, int width = 1920, int height = 1080);
    ~Window();

    bool Create();
    void Show();
    void Hide();
    bool PollEvents();
    void Close();

    // DirectX 12 specific methods
    void EnableDirectX12();
    void SetVSync(bool enabled) { m_vsyncEnabled = enabled; }
    void SetFullscreen(bool fullscreen);
    void SetClientSize(int width, int height);

    // Getters for DirectX 12
    HWND GetHandle() const { return m_hwnd; }
    HDC GetDeviceContext() const { return m_hdc; }
    bool IsRunning() const { return m_isRunning; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    bool IsFullscreen() const { return m_isFullscreen; }
    bool IsVSyncEnabled() const { return m_vsyncEnabled; }

    // Window rect helpers for DX12 swap chain
    void GetWindowRect(int& x, int& y, int& width, int& height) const;
    void GetClientRect(int& width, int& height) const;

    // Setters
    void SetTitle(const std::string& title);
    void Resize(int width, int height);
};
