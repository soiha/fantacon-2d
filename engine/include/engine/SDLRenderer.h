#pragma once

#include "IRenderer.h"
#include <SDL.h>

namespace Engine {

// SDL2-based renderer implementation
class SDLRenderer : public IRenderer {
public:
    SDLRenderer() = default;
    ~SDLRenderer() override;

    // Non-copyable, non-movable (owns SDL resources)
    SDLRenderer(const SDLRenderer&) = delete;
    SDLRenderer& operator=(const SDLRenderer&) = delete;
    SDLRenderer(SDLRenderer&&) = delete;
    SDLRenderer& operator=(SDLRenderer&&) = delete;

    // IRenderer interface
    bool init(const std::string& title, int width, int height) override;
    void shutdown() override;
    bool isInitialized() const override { return renderer_ != nullptr; }

    void clear() override;
    void present() override;

    void renderSprite(const Sprite& sprite, const Vec2& layerOffset = Vec2{0.0f, 0.0f}) override;
    void renderTilemap(const Tilemap& tilemap, const Vec2& layerOffset = Vec2{0.0f, 0.0f}) override;
    void renderText(const Text& text, const Vec2& position) override;
    void renderPixelBuffer(const PixelBuffer& buffer, const Vec2& layerOffset = Vec2{0.0f, 0.0f}) override;
    void renderIndexedPixelBuffer(const IndexedPixelBuffer& buffer, const Vec2& layerOffset = Vec2{0.0f, 0.0f}) override;

    TexturePtr createStreamingTexture(int width, int height) override;
    void updateTexture(Texture& texture, const Color* pixels, int width, int height) override;

    void* getBackendContext() override { return renderer_; }

    // SDL-specific accessors
    SDL_Renderer* getSDLRenderer() const { return renderer_; }
    SDL_Window* getWindow() const { return window_; }

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
};

} // namespace Engine
