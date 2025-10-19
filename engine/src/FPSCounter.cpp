#include "engine/FPSCounter.h"
#include <numeric>
#include <algorithm>

namespace Engine {

FPSCounter::FPSCounter(int sampleSize)
    : sampleSize_(sampleSize)
    , lastFrameTime_(SDL_GetTicks())
    , currentFPS_(0.0f)
    , frameTime_(0.0f)
    , deltaTime_(0.0f) {
}

void FPSCounter::update() {
    Uint32 currentTime = SDL_GetTicks();
    Uint32 frameDelta = currentTime - lastFrameTime_;
    lastFrameTime_ = currentTime;

    // Store frame time
    frameTimes_.push_back(frameDelta);

    // Keep only the last N samples
    if (frameTimes_.size() > static_cast<size_t>(sampleSize_)) {
        frameTimes_.pop_front();
    }

    // Calculate average frame time
    if (!frameTimes_.empty()) {
        float sum = std::accumulate(frameTimes_.begin(), frameTimes_.end(), 0.0f);
        frameTime_ = sum / frameTimes_.size();

        // Calculate FPS (avoid division by zero)
        if (frameTime_ > 0.0f) {
            currentFPS_ = 1000.0f / frameTime_;
        }
    }

    // Delta time in seconds
    deltaTime_ = frameDelta / 1000.0f;
}

void FPSCounter::reset() {
    frameTimes_.clear();
    lastFrameTime_ = SDL_GetTicks();
    currentFPS_ = 0.0f;
    frameTime_ = 0.0f;
    deltaTime_ = 0.0f;
}

} // namespace Engine
