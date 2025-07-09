#include "Window.h"
#include "Config.h"
#include <iostream>

Window::~Window() {
    Destroy();
}

bool Window::Create(const EngineConfig& config) {
    m_config = &config;

    if (!RegisterWindowClass()) {
        std::cerr << "Failed to register window class" << std::endl;
        return false;
    }

    RECT windowRect = { 0, 0, static_cast<LONG>(m_config->windowWidth), static_cast<LONG>(m_config->windowHeight) };
    DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&windowRect, windowStyle, FALSE);

    m_windowedStyle = windowStyle;

    m_hwnd = CreateWindowA(
        "GameWindowClass",
        m_config->windowTitle.c_str(),
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

    CenterWindow();
    GetWindowRect(m_hwnd, &m_windowedRect);

    if (m_config->fullscreen) {
        SetFullscreen(true);
    }

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    std::cout << "Window created successfully: " << m_config->windowWidth << "x" << m_config->windowHeight << std::endl;
    return true;
}

void Window::Destroy() {
    // Clear the resize callback to prevent it from being called during destruction
    m_resizeCallback = nullptr;

    if (m_hwnd) {
        // Remove the window user data to prevent message handling
        SetWindowLongPtr(m_hwnd, GWLP_USERDATA, 0);

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

bool Window::ChangeResolution(UINT width, UINT height) {
    if (!m_hwnd) return false;

    const_cast<EngineConfig*>(m_config)->windowWidth = width;
    const_cast<EngineConfig*>(m_config)->windowHeight = height;

    if (m_config->fullscreen) {
        return true;
    } else {
        RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        AdjustWindowRect(&windowRect, m_windowedStyle, FALSE);

        int windowWidth = windowRect.right - windowRect.left;
        int windowHeight = windowRect.bottom - windowRect.top;

        UINT screenWidth = GetSystemMetrics(SM_CXSCREEN);
        UINT screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int x = (screenWidth - windowWidth) / 2;
        int y = (screenHeight - windowHeight) / 2;

        SetWindowPos(m_hwnd, nullptr, x, y, windowWidth, windowHeight, SWP_NOZORDER | SWP_NOACTIVATE);
        GetWindowRect(m_hwnd, &m_windowedRect);
    }

    if (m_resizeCallback) {
        m_resizeCallback(width, height);
    }

    return true;
}

bool Window::SetFullscreen(bool fullscreen) {
    if (!m_hwnd || m_config->fullscreen == fullscreen) {
        return true;
    }

    if (fullscreen) {
        GetWindowRect(m_hwnd, &m_windowedRect);
        SetWindowLong(m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

        UINT screenWidth = GetSystemMetrics(SM_CXSCREEN);
        UINT screenHeight = GetSystemMetrics(SM_CYSCREEN);

        SetWindowPos(m_hwnd, HWND_TOP, 0, 0, screenWidth, screenHeight, SWP_FRAMECHANGED | SWP_NOACTIVATE);

        const_cast<EngineConfig*>(m_config)->fullscreen = true;
        const_cast<EngineConfig*>(m_config)->windowWidth = screenWidth;
        const_cast<EngineConfig*>(m_config)->windowHeight = screenHeight;

        if (m_resizeCallback) {
            m_resizeCallback(screenWidth, screenHeight);
        }
    } else {
        SetWindowLong(m_hwnd, GWL_STYLE, m_windowedStyle);

        // Validate the stored rect before using it
        int width = m_windowedRect.right - m_windowedRect.left;
        int height = m_windowedRect.bottom - m_windowedRect.top;

        if (width <= 0 || height <= 0) {
            // Use default size and center it
            width = 800;
            height = 600;
            UINT screenWidth = GetSystemMetrics(SM_CXSCREEN);
            UINT screenHeight = GetSystemMetrics(SM_CYSCREEN);
            int x = (screenWidth - width) / 2;
            int y = (screenHeight - height) / 2;

            SetWindowPos(m_hwnd, nullptr, x, y, width, height,
                        SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOZORDER);
        } else {
            SetWindowPos(m_hwnd, nullptr,
                        m_windowedRect.left, m_windowedRect.top,
                        width, height,
                        SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOZORDER);
        }

        // Force the window to be visible and redraw
        ShowWindow(m_hwnd, SW_SHOW);
        InvalidateRect(m_hwnd, nullptr, TRUE);
        UpdateWindow(m_hwnd);

        const_cast<EngineConfig*>(m_config)->fullscreen = false;

        RECT clientRect;
        GetClientRect(m_hwnd, &clientRect);
        const_cast<EngineConfig*>(m_config)->windowWidth = clientRect.right - clientRect.left;
        const_cast<EngineConfig*>(m_config)->windowHeight = clientRect.bottom - clientRect.top;

        if (m_resizeCallback) {
            m_resizeCallback(m_config->windowWidth, m_config->windowHeight);
        }
    }

    return true;
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Window* window = nullptr;

    if (msg == WM_CREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = reinterpret_cast<Window*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
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
        return 0;

    case WM_SYSKEYDOWN:
        if (wParam == VK_RETURN && window) {
            window->SetFullscreen(!window->m_config->fullscreen);
            return 0;
        }
        break;

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

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU) {
            return 0;
        }
        break;
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
    UINT newWidth = clientRect.right - clientRect.left;
    UINT newHeight = clientRect.bottom - clientRect.top;

    if (newWidth != m_config->windowWidth || newHeight != m_config->windowHeight) {
        const_cast<EngineConfig*>(m_config)->windowWidth = newWidth;
        const_cast<EngineConfig*>(m_config)->windowHeight = newHeight;

        if (m_resizeCallback) {
            m_resizeCallback(m_config->windowWidth, m_config->windowHeight);
        }
    }
}

void Window::CenterWindow() {
    if (!m_hwnd) return;

    RECT windowRect;
    GetWindowRect(m_hwnd, &windowRect);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    UINT screenWidth = GetSystemMetrics(SM_CXSCREEN);
    UINT screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    SetWindowPos(m_hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}
