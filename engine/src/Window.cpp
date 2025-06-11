#include "Window.h"
#include <iostream>

Window::~Window() {
    Destroy();
}

bool Window::Create(const EngineConfig& config) {
    if (!RegisterWindowClass()) {
        std::cerr << "Failed to register window class" << std::endl;
        return false;
    }

    // Get initial settings from passed config
    m_width = config.windowWidth;
    m_height = config.windowHeight;
    m_isFullscreen = config.fullscreen;

    // Calculate window size (accounting for borders)
    RECT windowRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
    DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&windowRect, windowStyle, FALSE);

    // Store windowed style for fullscreen toggle
    m_windowedStyle = windowStyle;

    // Create window
    m_hwnd = CreateWindowA(
        "GameWindowClass",
        config.windowTitle.c_str(),
        windowStyle,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr, nullptr,
        GetModuleHandle(nullptr),
        this
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
        // Create a mutable copy for SetFullscreen
        EngineConfig mutableConfig = config;
        SetFullscreen(true, mutableConfig);
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

void Window::GetScreenDimensions(uint32_t& width, uint32_t& height) const {
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
}

bool Window::ChangeResolution(uint32_t width, uint32_t height, EngineConfig& config) {
    if (!m_hwnd) return false;

    // Update passed config
    config.windowWidth = width;
    config.windowHeight = height;

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
        uint32_t screenWidth = 1;
        uint32_t screenHeight = 1;
        GetScreenDimensions(screenWidth, screenHeight);
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

bool Window::SetFullscreen(bool fullscreen, EngineConfig& config) {
    if (!m_hwnd || m_isFullscreen == fullscreen) {
        return true; // Already in desired state
    }

    if (fullscreen) {
        // Store current windowed position
        GetWindowRect(m_hwnd, &m_windowedRect);

        // Remove window decorations
        SetWindowLong(m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

        // Set window to cover entire screen
        uint32_t screenWidth = 1;
        uint32_t screenHeight = 1;
        GetScreenDimensions(screenWidth, screenHeight);
        SetWindowPos(m_hwnd, HWND_TOP, 0, 0,
                    screenWidth, screenHeight,
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

    // Update config
    config.fullscreen = fullscreen;
    return true;
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
        // Note: Alt+Enter handling would need config access -
        // consider using a callback or event system for this
        return 0;

    case WM_SIZE:
        if (window && wParam != SIZE_MINIMIZED) {
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
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "GameWindowClass";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    return RegisterClassExA(&wc) != 0;
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
    uint32_t screenWidth = 1;
    uint32_t screenHeight = 1;
    GetScreenDimensions(screenWidth, screenHeight);

    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    SetWindowPos(m_hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}
