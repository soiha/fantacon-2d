#pragma once

#include <SDL.h>
#include <deque>

namespace Engine {

class FPSCounter {
public:
    FPSCounter(int sampleSize = 60);

    // Call this once per frame
    void update();

    // Get current FPS
    float getFPS() const { return currentFPS_; }

    // Get current frame time in milliseconds
    float getFrameTime() const { return frameTime_; }

    // Get delta time in seconds (time since last frame)
    float getDeltaTime() const { return deltaTime_; }

    // Reset the counter
    void reset();

private:
    int sampleSize_;
    std::deque<Uint32> frameTimes_;
    Uint32 lastFrameTime_;
    float currentFPS_;
    float frameTime_;
    float deltaTime_;
};

} // namespace Engine
