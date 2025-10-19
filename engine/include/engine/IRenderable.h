#pragma once

namespace Engine {

class IRenderer;

// Interface for GameObjects with custom rendering
class IRenderable {
public:
    virtual ~IRenderable() = default;

    // Custom rendering (called after layer rendering)
    virtual void render(IRenderer& renderer) = 0;

    // Render order (lower values render first, higher values on top)
    virtual int getRenderOrder() const { return 0; }
};

} // namespace Engine
