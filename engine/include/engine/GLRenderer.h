#pragma once

#include "IRenderer.h"
#include <SDL.h>
#include <string>

namespace Engine {

// OpenGL-based renderer implementation with shader support
class GLRenderer : public IRenderer {
public:
    GLRenderer() = default;
    ~GLRenderer() override;

    // Non-copyable, non-movable (owns OpenGL resources)
    GLRenderer(const GLRenderer&) = delete;
    GLRenderer& operator=(const GLRenderer&) = delete;
    GLRenderer(GLRenderer&&) = delete;
    GLRenderer& operator=(GLRenderer&&) = delete;

    // IRenderer interface
    bool init(const std::string& title, int width, int height) override;
    void shutdown() override;
    bool isInitialized() const override { return glContext_ != nullptr; }

    void clear() override;
    void present() override;

    void renderSprite(const Sprite& sprite, const Vec2& layerOffset = Vec2{0.0f, 0.0f}, float opacity = 1.0f) override;
    void renderTilemap(const Tilemap& tilemap, const Vec2& layerOffset = Vec2{0.0f, 0.0f}, float opacity = 1.0f) override;
    void renderText(const Text& text, const Vec2& position, float opacity = 1.0f) override;
    void renderPixelBuffer(const PixelBuffer& buffer, const Vec2& layerOffset = Vec2{0.0f, 0.0f}, float opacity = 1.0f) override;
    void renderIndexedPixelBuffer(const IndexedPixelBuffer& buffer, const Vec2& layerOffset = Vec2{0.0f, 0.0f}, float opacity = 1.0f) override;

    TexturePtr createStreamingTexture(int width, int height) override;
    void updateTexture(Texture& texture, const Color* pixels, int width, int height) override;

    void* getBackendContext() override { return glContext_; }

    // GL-specific accessors
    SDL_Window* getWindow() const { return window_; }
    SDL_GLContext getGLContext() const { return glContext_; }

    // Viewport dimensions
    int getViewportWidth() const override { return windowWidth_; }
    int getViewportHeight() const override { return windowHeight_; }

    // IndexedPixelBuffer GL-specific support
    unsigned int createIndexedTexture(int width, int height);
    unsigned int createPaletteTexture();
    void updateIndexedTexture(unsigned int textureId, const uint8_t* indices, int width, int height);
    void updatePaletteTexture(unsigned int textureId, const Color* palette);

private:
    bool initOpenGL();
    bool compileShaders();
    void renderQuad(unsigned int texture, const Vec2& position, const Vec2& size, float rotation = 0.0f);
    void renderIndexedQuad(unsigned int indexTexture, unsigned int paletteTexture,
                          const Vec2& position, const Vec2& size, float opacity = 1.0f);

    SDL_Window* window_ = nullptr;
    SDL_GLContext glContext_ = nullptr;
    int windowWidth_ = 0;
    int windowHeight_ = 0;

    // Shader programs
    unsigned int textureShaderProgram_ = 0;
    unsigned int paletteShaderProgram_ = 0;

    // VAO/VBO for quad rendering
    unsigned int quadVAO_ = 0;
    unsigned int quadVBO_ = 0;
};

} // namespace Engine
