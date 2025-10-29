#pragma once

#include "IRenderer.h"

namespace Engine {

// Interface for objects that can be attached to a Layer
// All objects implementing this interface will automatically render
// when the layer is rendered
class ILayerAttachable {
public:
    virtual ~ILayerAttachable() = default;

    // Render this object using the given renderer
    // layerOffset: offset from the layer's position
    // opacity: layer opacity (0.0 = fully transparent, 1.0 = fully opaque)
    virtual void render(IRenderer& renderer, const Vec2& layerOffset, float opacity) = 0;

    // Check if this object should be rendered
    virtual bool isVisible() const = 0;
};

} // namespace Engine
