#pragma once

#include <chrono>
#include <thread>

namespace FPSUtils {
    // Limits frame rate to target FPS (does nothing if VSync is enabled)
    inline void LimitFrameRate(int targetFPS) {
        if (targetFPS <= 0) return;

        static LARGE_INTEGER frequency;
        static LARGE_INTEGER lastFrame;
        static bool initialized = false;

        if (!initialized) {
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&lastFrame);
            initialized = true;
            return;
        }

        LARGE_INTEGER targetTicks;
        targetTicks.QuadPart = frequency.QuadPart / targetFPS;

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);

        LARGE_INTEGER elapsed;
        elapsed.QuadPart = now.QuadPart - lastFrame.QuadPart;

        if (elapsed.QuadPart < targetTicks.QuadPart) {
            // Busy wait for precise timing
            while (elapsed.QuadPart < targetTicks.QuadPart) {
                QueryPerformanceCounter(&now);
                elapsed.QuadPart = now.QuadPart - lastFrame.QuadPart;
            }
        }

        lastFrame.QuadPart += targetTicks.QuadPart;
    }

    // Call this every frame to update FPS counter, returns true when FPS updates
    inline bool UpdateFPSCounter(float& outFPS, int updateIntervalMS = 500) {
        static auto lastTime = std::chrono::high_resolution_clock::now();
        static int frameCount = 0;
        static float fps = 0.0f;

        frameCount++;
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = now - lastTime;

        if (elapsed >= std::chrono::milliseconds(updateIntervalMS)) {
            fps = frameCount / std::chrono::duration<float>(elapsed).count();
            fps = std::round(fps);
            frameCount = 0;
            lastTime = now;
            outFPS = fps;
            return true; // FPS was updated
        }

        outFPS = fps;
        return false; // FPS not updated this frame
    }
} // namespace FPSUtils
