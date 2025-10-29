#pragma once

#include "Types.h"
#include "Texture.h"
#include <string>
#include <memory>

namespace Engine {

class Sprite;
class Tilemap;
class Text;
class PixelBuffer;
class IndexedPixelBuffer;

// Abstract renderer interface - can be implemented for SDL, Vulkan, headless, etc.
class IRenderer {
public:
    virtual ~IRenderer() = default;

    // Initialization
    virtual bool init(const std::string& title, int width, int height) = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;

    // Frame rendering
    virtual void clear() = 0;
    virtual void present() = 0;

    // Entity rendering
    virtual void renderSprite(const Sprite& sprite, const Vec2& layerOffset = Vec2{0.0f, 0.0f}, float opacity = 1.0f) = 0;
    virtual void renderTilemap(const Tilemap& tilemap, const Vec2& layerOffset = Vec2{0.0f, 0.0f}, float opacity = 1.0f) = 0;
    virtual void renderText(const Text& text, const Vec2& position, float opacity = 1.0f) = 0;
    virtual void renderPixelBuffer(const PixelBuffer& buffer, const Vec2& layerOffset = Vec2{0.0f, 0.0f}, float opacity = 1.0f) = 0;
    virtual void renderIndexedPixelBuffer(const IndexedPixelBuffer& buffer, const Vec2& layerOffset = Vec2{0.0f, 0.0f}, float opacity = 1.0f) = 0;

    // Streaming texture support (for PixelBuffer)
    virtual TexturePtr createStreamingTexture(int width, int height) = 0;
    virtual void updateTexture(Texture& texture, const Color* pixels, int width, int height) = 0;

    // Backend-specific context (for texture loading, etc.)
    virtual void* getBackendContext() = 0;

    // Viewport dimensions
    virtual int getViewportWidth() const = 0;
    virtual int getViewportHeight() const = 0;
};

using RendererPtr = std::unique_ptr<IRenderer>;

} // namespace Engine
