#pragma once

namespace Engine {

// Interface for GameObjects that need per-frame updates with variable delta time
class IUpdateable {
public:
    virtual ~IUpdateable() = default;

    // Called every frame with time elapsed since last frame (in seconds)
    virtual void update(float deltaTime) = 0;
};

// Interface for GameObjects that need fixed-rate updates (physics, etc.)
class IFixedUpdateable {
public:
    virtual ~IFixedUpdateable() = default;

    // Called at fixed intervals (default 60Hz = 0.0166s)
    virtual void fixedUpdate(float fixedDeltaTime) = 0;
};

} // namespace Engine
