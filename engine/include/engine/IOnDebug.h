#pragma once

namespace Engine {

class IRenderer;

// Interface for GameObjects that provide debug visualization
class IOnDebug {
public:
    virtual ~IOnDebug() = default;

    // Called during debug rendering pass (when debug mode is enabled)
    virtual void onDebug(IRenderer& renderer) = 0;
};

} // namespace Engine
