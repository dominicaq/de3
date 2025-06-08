#include "Window.h"
#include "Config.h"
#include <iostream>

Window::~Window() {
    Destroy();
}

bool Window::Create() {
    if (!RegisterWindowClass()) {
        std::cerr << "Failed to register window class" << std::endl;
        return false;
    }

    // Get initial settings from global config
    m_width = g_config.windowWidth;
    m_height = g_config.windowHeight;
    m_isFullscreen = g_config.fullscreen;

    // Convert string to wstring for title
    std::wstring title(g_config.windowTitle.begin(), g_config.windowTitle.end());

    // Calculate window size (accounting for borders)
    RECT windowRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
    DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&windowRect, windowStyle, FALSE);

    // Store windowed style for fullscreen toggle
    m_windowedStyle = windowStyle;

    // Create window
    m_hwnd = CreateWindowExW(
        0,
        L"GameWindowClass",
        title.c_str(),
        windowStyle,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr, nullptr,
        GetModuleHandle(nullptr),
        this  // Pass this pointer for WindowProc
    );

    if (!m_hwnd) {
        std::cerr << "Failed to create window. Error: " << GetLastError() << std::endl;
        return false;
    }

    // Center the window
    CenterWindow();

    // Store windowed position for fullscreen toggle
    GetWindowRect(m_hwnd, &m_windowedRect);

    // Set fullscreen if requested
    if (m_isFullscreen) {
        SetFullscreen(true);
    }

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    std::cout << "Window created successfully: " << m_width << "x" << m_height << std::endl;
    return true;
}

void Window::Destroy() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    UnregisterClassW(L"GameWindowClass", GetModuleHandle(nullptr));
}

void Window::ProcessEvents() {
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

uint32_t Window::GetScreenWidth() const {
    return GetSystemMetrics(SM_CXSCREEN);
}

uint32_t Window::GetScreenHeight() const {
    return GetSystemMetrics(SM_CYSCREEN);
}

void Window::GetScreenDimensions(uint32_t& width, uint32_t& height) const {
    width = GetScreenWidth();
    height = GetScreenHeight();
}

bool Window::ChangeResolution(uint32_t width, uint32_t height) {
    if (!m_hwnd) return false;

    // Update global config first
    g_config.windowWidth = width;
    g_config.windowHeight = height;

    m_width = width;
    m_height = height;

    if (m_isFullscreen) {
        // In fullscreen, just update our tracking variables
        // The actual display change happens in SetFullscreen
        return true;
    } else {
        // Resize windowed mode
        RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        AdjustWindowRect(&windowRect, m_windowedStyle, FALSE);

        int windowWidth = windowRect.right - windowRect.left;
        int windowHeight = windowRect.bottom - windowRect.top;

        // Center the resized window
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        int x = (screenWidth - windowWidth) / 2;
        int y = (screenHeight - windowHeight) / 2;

        SetWindowPos(m_hwnd, nullptr, x, y, windowWidth, windowHeight,
                    SWP_NOZORDER | SWP_NOACTIVATE);

        // Update stored windowed rect
        GetWindowRect(m_hwnd, &m_windowedRect);
    }

    std::cout << "Resolution changed to: " << width << "x" << height << std::endl;
    return true;
}

bool Window::SetFullscreen(bool fullscreen) {
    if (!m_hwnd || m_isFullscreen == fullscreen) {
        return true; // Already in desired state
    }

    if (fullscreen) {
        // Store current windowed position
        GetWindowRect(m_hwnd, &m_windowedRect);

        // Remove window decorations
        SetWindowLong(m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

        // Set window to cover entire screen
        SetWindowPos(m_hwnd, HWND_TOP, 0, 0,
                    GetScreenWidth(), GetScreenHeight(),
                    SWP_FRAMECHANGED | SWP_NOACTIVATE);

        m_isFullscreen = true;
        std::cout << "Switched to fullscreen mode" << std::endl;
    } else {
        // Restore window decorations
        SetWindowLong(m_hwnd, GWL_STYLE, m_windowedStyle);

        // Restore windowed position and size
        SetWindowPos(m_hwnd, nullptr,
                    m_windowedRect.left, m_windowedRect.top,
                    m_windowedRect.right - m_windowedRect.left,
                    m_windowedRect.bottom - m_windowedRect.top,
                    SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOZORDER);

        m_isFullscreen = false;
        std::cout << "Switched to windowed mode" << std::endl;
    }

    // Update global config
    g_config.fullscreen = fullscreen;
    return true;
}

void Window::SetVSync(bool enabled) {
    g_config.vsync = enabled;
    std::cout << "VSync " << (enabled ? "enabled" : "disabled") << std::endl;
    // Note: DX12 swap chain will need to be recreated to apply VSync changes
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Window* window = nullptr;

    if (msg == WM_CREATE) {
        // Get the Window pointer from CreateWindowEx
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = reinterpret_cast<Window*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
        // Retrieve the Window pointer
        window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        if (window) {
            window->m_shouldClose = true;
        }
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE && window) {
            window->Close();
        }
        // Alt+Enter for fullscreen toggle
        if (wParam == VK_RETURN && (GetKeyState(VK_MENU) & 0x8000) && window) {
            window->SetFullscreen(!window->IsFullscreen());
        }
        return 0;

    case WM_SIZE:
        if (window && wParam != SIZE_MINIMIZED) {
            // Update our stored size when window is resized
            // (This won't happen due to our window style, but good to have)
            window->UpdateWindowSize();
        }
        return 0;

    case WM_CLOSE:
        if (window) {
            window->Close();
        }
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool Window::RegisterWindowClass() {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"GameWindowClass";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    return RegisterClassExW(&wc) != 0;
}

void Window::UpdateWindowSize() {
    if (!m_hwnd) return;

    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    m_width = clientRect.right - clientRect.left;
    m_height = clientRect.bottom - clientRect.top;
}

void Window::CenterWindow() {
    if (!m_hwnd) return;

    RECT windowRect;
    GetWindowRect(m_hwnd, &windowRect);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    SetWindowPos(m_hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}
