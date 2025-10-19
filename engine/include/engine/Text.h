#pragma once

#include "Types.h"
#include "Font.h"
#include <SDL.h>
#include <string>
#include <memory>

namespace Engine {

class IRenderer;

// Helper function to convert our Color to SDL_Color
inline SDL_Color colorToSDL(const Color& c) {
    return {c.r, c.g, c.b, c.a};
}

class Text {
public:
    Text() = default;
    ~Text();

    // Non-copyable but movable
    Text(const Text&) = delete;
    Text& operator=(const Text&) = delete;
    Text(Text&& other) noexcept;
    Text& operator=(Text&& other) noexcept;

    // Set the font to use (from ResourceManager)
    void setFont(FontPtr font) { font_ = font; }
    FontPtr getFont() const { return font_; }

    // Set the text to render
    void setText(const std::string& text, IRenderer& renderer, const Color& color = Color{255, 255, 255, 255});

    // Get texture for rendering (after setText)
    SDL_Texture* getTexture() const { return texture_; }

    // Get dimensions
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

    // Check if ready to render
    bool isValid() const { return texture_ != nullptr; }

private:
    void cleanup();

    FontPtr font_;  // Shared font resource
    SDL_Texture* texture_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

using TextPtr = std::shared_ptr<Text>;

} // namespace Engine
