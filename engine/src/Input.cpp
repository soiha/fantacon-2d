#include "engine/Input.h"

namespace Engine {

bool Input::isKeyPressed(SDL_Keycode key) const {
    return keysPressed_.find(key) != keysPressed_.end();
}

bool Input::isKeyHeld(SDL_Keycode key) const {
    return keysHeld_.find(key) != keysHeld_.end();
}

bool Input::isKeyReleased(SDL_Keycode key) const {
    return keysReleased_.find(key) != keysReleased_.end();
}

bool Input::isMouseButtonPressed(int button) const {
    return mouseButtonsPressed_.find(button) != mouseButtonsPressed_.end();
}

bool Input::isMouseButtonHeld(int button) const {
    return mouseButtonsHeld_.find(button) != mouseButtonsHeld_.end();
}

bool Input::isMouseButtonReleased(int button) const {
    return mouseButtonsReleased_.find(button) != mouseButtonsReleased_.end();
}

void Input::processEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_KEYDOWN:
            if (!event.key.repeat) {  // Ignore key repeats
                keysPressed_.insert(event.key.keysym.sym);
                keysHeld_.insert(event.key.keysym.sym);
            }
            break;

        case SDL_KEYUP:
            keysReleased_.insert(event.key.keysym.sym);
            keysHeld_.erase(event.key.keysym.sym);
            break;

        case SDL_MOUSEMOTION:
            mousePosition_.x = static_cast<float>(event.motion.x);
            mousePosition_.y = static_cast<float>(event.motion.y);
            mouseDelta_.x = static_cast<float>(event.motion.xrel);
            mouseDelta_.y = static_cast<float>(event.motion.yrel);
            break;

        case SDL_MOUSEBUTTONDOWN:
            mouseButtonsPressed_.insert(event.button.button);
            mouseButtonsHeld_.insert(event.button.button);
            break;

        case SDL_MOUSEBUTTONUP:
            mouseButtonsReleased_.insert(event.button.button);
            mouseButtonsHeld_.erase(event.button.button);
            break;

        case SDL_MOUSEWHEEL:
            mouseWheel_.x = static_cast<float>(event.wheel.x);
            mouseWheel_.y = static_cast<float>(event.wheel.y);
            break;

        default:
            break;
    }
}

void Input::update() {
    // Clear per-frame state
    keysPressed_.clear();
    keysReleased_.clear();
    mouseButtonsPressed_.clear();
    mouseButtonsReleased_.clear();

    // Reset mouse delta and wheel
    mouseDelta_ = {0, 0};
    mouseWheel_ = {0, 0};

    // Update last mouse position
    lastMousePosition_ = mousePosition_;
}

} // namespace Engine
