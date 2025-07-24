#pragma once
#include <windows.h>

namespace InputKeys {
    // Letter keys
    enum Key : int {
        A = 0x41, B = 0x42, C = 0x43, D = 0x44, E = 0x45, F = 0x46, G = 0x47, H = 0x48,
        I = 0x49, J = 0x4A, K = 0x4B, L = 0x4C, M = 0x4D, N = 0x4E, O = 0x4F, P = 0x50,
        Q = 0x51, R = 0x52, S = 0x53, T = 0x54, U = 0x55, V = 0x56, W = 0x57, X = 0x58,
        Y = 0x59, Z = 0x5A,

        // Number keys (top row)
        NUM_0 = 0x30, NUM_1 = 0x31, NUM_2 = 0x32, NUM_3 = 0x33, NUM_4 = 0x34,
        NUM_5 = 0x35, NUM_6 = 0x36, NUM_7 = 0x37, NUM_8 = 0x38, NUM_9 = 0x39,

        // Function keys
        F1 = VK_F1, F2 = VK_F2, F3 = VK_F3, F4 = VK_F4, F5 = VK_F5, F6 = VK_F6,
        F7 = VK_F7, F8 = VK_F8, F9 = VK_F9, F10 = VK_F10, F11 = VK_F11, F12 = VK_F12,

        // Arrow keys
        ARROW_LEFT = VK_LEFT,
        ARROW_RIGHT = VK_RIGHT,
        ARROW_UP = VK_UP,
        ARROW_DOWN = VK_DOWN,

        // Special keys
        SPACE = VK_SPACE,
        ENTER = VK_RETURN,
        ESCAPE = VK_ESCAPE,
        TAB = VK_TAB,
        BACKSPACE = VK_BACK,
        DELETE_KEY = VK_DELETE,
        INSERT = VK_INSERT,
        HOME = VK_HOME,
        END = VK_END,
        PAGE_UP = VK_PRIOR,
        PAGE_DOWN = VK_NEXT,

        // Modifier keys
        SHIFT = VK_SHIFT,
        CTRL = VK_CONTROL,
        ALT = VK_MENU,
        LEFT_SHIFT = VK_LSHIFT,
        RIGHT_SHIFT = VK_RSHIFT,
        LEFT_CTRL = VK_LCONTROL,
        RIGHT_CTRL = VK_RCONTROL,
        LEFT_ALT = VK_LMENU,
        RIGHT_ALT = VK_RMENU,

        // Numpad keys
        NUMPAD_0 = VK_NUMPAD0, NUMPAD_1 = VK_NUMPAD1, NUMPAD_2 = VK_NUMPAD2,
        NUMPAD_3 = VK_NUMPAD3, NUMPAD_4 = VK_NUMPAD4, NUMPAD_5 = VK_NUMPAD5,
        NUMPAD_6 = VK_NUMPAD6, NUMPAD_7 = VK_NUMPAD7, NUMPAD_8 = VK_NUMPAD8,
        NUMPAD_9 = VK_NUMPAD9,
        NUMPAD_MULTIPLY = VK_MULTIPLY,
        NUMPAD_ADD = VK_ADD,
        NUMPAD_SUBTRACT = VK_SUBTRACT,
        NUMPAD_DECIMAL = VK_DECIMAL,
        NUMPAD_DIVIDE = VK_DIVIDE,

        // Additional common keys
        CAPS_LOCK = VK_CAPITAL,
        NUM_LOCK = VK_NUMLOCK,
        SCROLL_LOCK = VK_SCROLL,
        PRINT_SCREEN = VK_SNAPSHOT,
        PAUSE = VK_PAUSE,

        // Symbol keys
        MINUS = VK_OEM_MINUS,        // - key
        EQUALS = VK_OEM_PLUS,        // = key
        LEFT_BRACKET = VK_OEM_4,     // [ key
        RIGHT_BRACKET = VK_OEM_6,    // ] key
        SEMICOLON = VK_OEM_1,        // ; key
        APOSTROPHE = VK_OEM_7,       // ' key
        GRAVE = VK_OEM_3,            // ` key
        BACKSLASH = VK_OEM_5,        // \ key
        COMMA = VK_OEM_COMMA,        // , key
        PERIOD = VK_OEM_PERIOD,      // . key
        SLASH = VK_OEM_2             // / key
    };

    // Mouse buttons
    enum MouseButton : int {
        MOUSE_LEFT = 0,
        MOUSE_RIGHT = 1,
        MOUSE_MIDDLE = 2,
        MOUSE_X1 = 3,
        MOUSE_X2 = 4
    };
}
