#pragma once

namespace Engine {

class Input;

// Interface for GameObjects that handle input
class IInputHandler {
public:
    virtual ~IInputHandler() = default;

    // Called each frame with current input state
    virtual void handleInput(const Input& input) = 0;
};

} // namespace Engine
