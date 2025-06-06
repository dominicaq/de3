#include "Window.h"
#include <iostream>
#include <stdexcept>

Window::Window(const std::string& title, int width, int height)
    : m_hwnd(nullptr)
    , m_hinstance(GetModuleHandle(nullptr))
    , m_hdc(nullptr)
    , m_className("DX12WindowClass")
    , m_windowTitle(title)
    , m_width(width)
    , m_height(height)
    , m_isRunning(false)
    , m_isFullscreen(false)
    , m_vsyncEnabled(true)
{
}

Window::~Window() {
    if (m_hdc) {
        ReleaseDC(m_hwnd, m_hdc);
    }
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
    UnregisterClassA(m_className.c_str(), m_hinstance);
}

bool Window::Create() {
    try {
        RegisterWindowClass();
        CreateWindowHandle();
        SetupForDirectX();

        m_isRunning = true;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create window: " << e.what() << std::endl;
        return false;
    }
}

void Window::RegisterWindowClass() {
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // CS_OWNDC important for DX12
    wc.lpfnWndProc = Window::WindowProc;
    wc.hInstance = m_hinstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; // Don't erase background - DX12 will handle this
    wc.lpszClassName = m_className.c_str();
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExA(&wc)) {
        throw std::runtime_error("Failed to register window class");
    }
}

void Window::CreateWindowHandle() {
    // Calculate window size to account for borders
    DWORD windowStyle = WS_OVERLAPPEDWINDOW;
    DWORD windowExStyle = WS_EX_APPWINDOW;

    RECT rect = { 0, 0, m_width, m_height };
    AdjustWindowRectEx(&rect, windowStyle, FALSE, windowExStyle);

    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;

    // Center window on screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    // Create window
    m_hwnd = CreateWindowExA(
        windowExStyle,
        m_className.c_str(),
        m_windowTitle.c_str(),
        windowStyle,
        x, y,
        windowWidth, windowHeight,
        nullptr,
        nullptr,
        m_hinstance,
        this
    );

    if (!m_hwnd) {
        throw std::runtime_error("Failed to create window handle");
    }
}

void Window::SetupForDirectX() {
    // Get device context for DirectX 12 compatibility
    m_hdc = GetDC(m_hwnd);
    if (!m_hdc) {
        throw std::runtime_error("Failed to get device context");
    }

    // Disable Windows composition for better performance with DX12
    DWORD dwAttribute = DWMNCRP_DISABLED;
    DwmSetWindowAttribute(m_hwnd, DWMWA_NCRENDERING_POLICY,
                         &dwAttribute, sizeof(DWORD));

    // Set high DPI awareness for crisp rendering
    SetProcessDPIAware();
}

void Window::EnableDirectX12() {
    // Additional setup specifically for DirectX 12
    if (!m_hwnd) return;

    // Ensure window is properly sized for DX12 swap chain
    RECT clientRect;
    ::GetClientRect(m_hwnd, &clientRect);
    m_width = clientRect.right - clientRect.left;
    m_height = clientRect.bottom - clientRect.top;

    // Force a redraw to clear any GDI artifacts
    InvalidateRect(m_hwnd, nullptr, TRUE);
    UpdateWindow(m_hwnd);
}

void Window::SetFullscreen(bool fullscreen) {
    if (m_isFullscreen == fullscreen) return;

    static RECT s_windowRect = {};
    static DWORD s_windowStyle = 0;

    if (fullscreen) {
        // Store current window state
        ::GetWindowRect(m_hwnd, &s_windowRect);
        s_windowStyle = GetWindowLong(m_hwnd, GWL_STYLE);

        // Get monitor info
        HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
        GetMonitorInfo(monitor, &monitorInfo);

        // Set fullscreen
        SetWindowLong(m_hwnd, GWL_STYLE, WS_POPUP);
        SetWindowPos(m_hwnd, HWND_TOP,
                     monitorInfo.rcMonitor.left,
                     monitorInfo.rcMonitor.top,
                     monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                     monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                     SWP_FRAMECHANGED | SWP_NOACTIVATE);

        m_width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
        m_height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
    } else {
        // Restore windowed mode
        SetWindowLong(m_hwnd, GWL_STYLE, s_windowStyle);
        SetWindowPos(m_hwnd, HWND_NOTOPMOST,
                     s_windowRect.left,
                     s_windowRect.top,
                     s_windowRect.right - s_windowRect.left,
                     s_windowRect.bottom - s_windowRect.top,
                     SWP_FRAMECHANGED | SWP_NOACTIVATE);

        RECT clientRect;
        ::GetClientRect(m_hwnd, &clientRect);
        m_width = clientRect.right - clientRect.left;
        m_height = clientRect.bottom - clientRect.top;
    }

    m_isFullscreen = fullscreen;
}

void Window::SetClientSize(int width, int height) {
    if (m_isFullscreen) return;

    RECT rect = { 0, 0, width, height };
    AdjustWindowRectEx(&rect, GetWindowLong(m_hwnd, GWL_STYLE), FALSE,
                       GetWindowLong(m_hwnd, GWL_EXSTYLE));

    SetWindowPos(m_hwnd, nullptr, 0, 0,
                 rect.right - rect.left,
                 rect.bottom - rect.top,
                 SWP_NOMOVE | SWP_NOZORDER);

    m_width = width;
    m_height = height;
}

void Window::GetWindowRect(int& x, int& y, int& width, int& height) const {
    RECT rect;
    ::GetWindowRect(m_hwnd, &rect);
    x = rect.left;
    y = rect.top;
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
}

void Window::GetClientRect(int& width, int& height) const {
    RECT rect;
    ::GetClientRect(m_hwnd, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
}

void Window::Show() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
    }
}

void Window::Hide() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_HIDE);
    }
}

bool Window::PollEvents() {
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            m_isRunning = false;
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return m_isRunning;
}

void Window::Close() {
    if (m_hwnd) {
        PostMessage(m_hwnd, WM_CLOSE, 0, 0);
    }
}

void Window::SetTitle(const std::string& title) {
    m_windowTitle = title;
    if (m_hwnd) {
        SetWindowTextA(m_hwnd, title.c_str());
    }
}

void Window::Resize(int width, int height) {
    SetClientSize(width, height);
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Window* window = nullptr;

    if (uMsg == WM_CREATE) {
        CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = reinterpret_cast<Window*>(createStruct->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
        window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (window) {
        return window->HandleMessage(uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT Window::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE:
        m_isRunning = false;
        DestroyWindow(m_hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            m_width = LOWORD(lParam);
            m_height = HIWORD(lParam);
            // Note: DirectX 12 swap chain resize should be handled by the renderer
        }
        return 0;

    case WM_ENTERSIZEMOVE:
        // Pause rendering during resize for better performance
        return 0;

    case WM_EXITSIZEMOVE:
        // Resume rendering after resize
        RECT clientRect;
        ::GetClientRect(m_hwnd, &clientRect);
        m_width = clientRect.right - clientRect.left;
        m_height = clientRect.bottom - clientRect.top;
        return 0;

    case WM_GETMINMAXINFO:
        // Set minimum window size
        {
            MINMAXINFO* minMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
            minMaxInfo->ptMinTrackSize.x = 320;
            minMaxInfo->ptMinTrackSize.y = 240;
        }
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            Close();
        } else if (wParam == VK_F11) {
            SetFullscreen(!m_isFullscreen);
        } else if (wParam == VK_F1) {
            SetVSync(!m_vsyncEnabled);
        }
        return 0;

    case WM_ERASEBKGND:
        // Don't erase background - DirectX 12 will handle this
        return 1;

    case WM_PAINT:
        // Minimal paint handling for DirectX 12
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_hwnd, &ps);
            // DirectX 12 will handle all rendering
            EndPaint(m_hwnd, &ps);
        }
        return 0;
    }

    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}
