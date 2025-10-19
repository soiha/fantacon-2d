#pragma once

#include "Types.h"
#include <SDL.h>
#include <unordered_map>
#include <unordered_set>

namespace Engine {

class Input {
public:
    Input() = default;

    // Keyboard state queries
    bool isKeyPressed(SDL_Keycode key) const;   // Pressed this frame
    bool isKeyHeld(SDL_Keycode key) const;      // Currently held down
    bool isKeyReleased(SDL_Keycode key) const;  // Released this frame

    // Mouse position
    Vec2 getMousePosition() const { return mousePosition_; }
    Vec2 getMouseDelta() const { return mouseDelta_; }

    // Mouse button state
    bool isMouseButtonPressed(int button) const;
    bool isMouseButtonHeld(int button) const;
    bool isMouseButtonReleased(int button) const;

    // Mouse wheel
    Vec2 getMouseWheel() const { return mouseWheel_; }

    // Internal methods (called by engine)
    void processEvent(const SDL_Event& event);
    void update();  // Called at end of frame to clear pressed/released states

private:
    // Keyboard state
    std::unordered_set<SDL_Keycode> keysPressed_;   // This frame
    std::unordered_set<SDL_Keycode> keysHeld_;      // Currently held
    std::unordered_set<SDL_Keycode> keysReleased_;  // This frame

    // Mouse state
    Vec2 mousePosition_{0, 0};
    Vec2 mouseDelta_{0, 0};
    Vec2 mouseWheel_{0, 0};
    Vec2 lastMousePosition_{0, 0};

    std::unordered_set<int> mouseButtonsPressed_;
    std::unordered_set<int> mouseButtonsHeld_;
    std::unordered_set<int> mouseButtonsReleased_;
};

} // namespace Engine
