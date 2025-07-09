#pragma once
#include <chrono>
#include <thread>

namespace FPSUtils {

    class FPSUtils {
    private:
        // Frame limiter state
        std::chrono::steady_clock::time_point nextFrame;
        bool limiterInitialized = false;

        // FPS counter state
        std::chrono::high_resolution_clock::time_point lastTime;
        int frameCount = 0;
        float fps = 0.0f;

    public:
        FPSUtils() : nextFrame(std::chrono::steady_clock::now()),
                     lastTime(std::chrono::high_resolution_clock::now()) {}

        void LimitFrameRate(int targetFPS) {
            if (targetFPS <= 0) return;

            if (!limiterInitialized) {
                nextFrame = std::chrono::steady_clock::now();
                limiterInitialized = true;
                return;
            }

            // Use microseconds for better precision
            nextFrame += std::chrono::microseconds(1000000 / targetFPS);
            std::this_thread::sleep_until(nextFrame);
        }

        bool UpdateFPSCounter(float& outFPS, int updateIntervalMS = 500) {
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
    };
}
