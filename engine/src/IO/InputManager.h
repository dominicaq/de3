#pragma once
#include <windows.h>
#include <iostream>
#include "InputKeys.h"

class InputManager {
public:
    enum class CursorMode {
        NORMAL,
        HIDDEN,
        DISABLED
    };

    static const int MAX_KEYS = 256;

    // Singleton access
    static InputManager& getInstance() {
        static InputManager instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    void init(HWND window) {
        m_window = window;
        for (int i = 0; i < MAX_KEYS; ++i) {
            m_keyStates[i] = false;
            m_prevKeyStates[i] = false;
        }
        m_lastX = 0;
        m_lastY = 0;
        m_cursorHidden = false;
        m_cursorClipped = false;
    }

    void update() {
        for (int key = 0; key < MAX_KEYS; ++key) {
            m_prevKeyStates[key] = m_keyStates[key];
            m_keyStates[key] = (GetAsyncKeyState(key) & 0x8000) != 0;
        }

        POINT mousePos;
        GetCursorPos(&mousePos);
        ScreenToClient(m_window, &mousePos);

        m_mouseX = mousePos.x;
        m_mouseY = mousePos.y;

        if (m_firstMouse) {
            m_lastX = m_mouseX;
            m_lastY = m_mouseY;
            m_firstMouse = false;
            m_xOffset = 0;
            m_yOffset = 0;
        } else {
            m_xOffset = m_mouseX - m_lastX;
            m_yOffset = m_lastY - m_mouseY;
        }

        // Always reset cursor to center when disabled (like GLFW)
        if (m_cursorMode == CursorMode::DISABLED) {
            RECT clientRect;
            GetClientRect(m_window, &clientRect);

            POINT centerPoint;
            centerPoint.x = (clientRect.right - clientRect.left) / 2;
            centerPoint.y = (clientRect.bottom - clientRect.top) / 2;

            // Convert to screen coordinates and reset cursor
            ClientToScreen(m_window, &centerPoint);
            SetCursorPos(centerPoint.x, centerPoint.y);

            // Update last position to center (in client coordinates)
            m_lastX = (clientRect.right - clientRect.left) / 2;
            m_lastY = (clientRect.bottom - clientRect.top) / 2;
        } else {
            // Normal mode: just track the cursor position
            m_lastX = m_mouseX;
            m_lastY = m_mouseY;
        }
    }

    bool isKeyDown(int key) const {
        if (key < 0 || key >= MAX_KEYS) {
            std::cerr << "[Warning] InputManager::isKeyDown(): invalid key: " << key << "\n";
            return false;
        }
        return m_keyStates[key];
    }

    bool isKeyPressed(int key) const {
        if (key < 0 || key >= MAX_KEYS) {
            std::cerr << "[Warning] InputManager::isKeyPressed(): invalid key: " << key << "\n";
            return false;
        }
        return m_keyStates[key] && !m_prevKeyStates[key];
    }

    bool isKeyReleased(int key) const {
        if (key < 0 || key >= MAX_KEYS) {
            std::cerr << "[Warning] InputManager::isKeyReleased(): invalid key: " << key << "\n";
            return false;
        }
        return !m_keyStates[key] && m_prevKeyStates[key];
    }

    void setCursorMode(CursorMode mode) {
        m_cursorMode = mode;

        switch (mode) {
            case CursorMode::NORMAL:
                ShowCursor(TRUE);
                ClipCursor(nullptr);
                m_cursorHidden = false;
                m_cursorClipped = false;
                break;

            case CursorMode::HIDDEN:
                ShowCursor(FALSE);
                ClipCursor(nullptr);
                m_cursorHidden = true;
                m_cursorClipped = false;
                break;

            case CursorMode::DISABLED:
                ShowCursor(FALSE);
                // Don't clip the cursor - we'll handle position ourselves
                ClipCursor(nullptr);
                m_cursorHidden = true;
                m_cursorClipped = false;

                // Reset first mouse flag when entering disabled mode
                m_firstMouse = true;
                break;
        }
    }

    float getMouseXOffset() const {
        return static_cast<float>(m_xOffset);
    }

    float getMouseYOffset() const {
        return static_cast<float>(m_yOffset);
    }

    bool isCursorDisabled() const {
        return m_cursorMode == CursorMode::DISABLED;
    }

    CursorMode getCursorMode() const {
        return m_cursorMode;
    }

    bool isMouseButtonDown(int button) const {
        int vkCode = getMouseVKCode(button);
        return vkCode != -1 && (GetAsyncKeyState(vkCode) & 0x8000) != 0;
    }

private:
    InputManager() = default;

    HWND m_window = nullptr;
    bool m_keyStates[MAX_KEYS];
    bool m_prevKeyStates[MAX_KEYS];
    bool m_firstMouse = true;
    long m_lastX, m_lastY, m_mouseX, m_mouseY;
    long m_xOffset, m_yOffset;
    CursorMode m_cursorMode = CursorMode::NORMAL;
    bool m_cursorHidden = false;
    bool m_cursorClipped = false;

    int getMouseVKCode(int button) const {
        switch (button) {
            case InputKeys::MOUSE_LEFT: return VK_LBUTTON;
            case InputKeys::MOUSE_RIGHT: return VK_RBUTTON;
            case InputKeys::MOUSE_MIDDLE: return VK_MBUTTON;
            case InputKeys::MOUSE_X1: return VK_XBUTTON1;
            case InputKeys::MOUSE_X2: return VK_XBUTTON2;
            default: return -1;
        }
    }
};

// Convenience macro for easy access
#define Input InputManager::getInstance()
