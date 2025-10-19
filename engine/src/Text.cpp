#include "engine/Text.h"
#include "engine/IRenderer.h"
#include <iostream>

namespace Engine {

Text::~Text() {
    cleanup();
}

Text::Text(Text&& other) noexcept
    : font_(std::move(other.font_))
    , texture_(other.texture_)
    , width_(other.width_)
    , height_(other.height_) {
    other.texture_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
}

Text& Text::operator=(Text&& other) noexcept {
    if (this != &other) {
        cleanup();
        font_ = std::move(other.font_);
        texture_ = other.texture_;
        width_ = other.width_;
        height_ = other.height_;
        other.texture_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;
    }
    return *this;
}

void Text::cleanup() {
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    // Font is managed via shared_ptr, no manual cleanup needed
}

void Text::setText(const std::string& text, IRenderer& renderer, const Color& color) {
    if (!font_ || !font_->isValid()) {
        std::cerr << "Font not set or invalid! Call setFont first." << std::endl;
        return;
    }

    // Clean up old texture
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }

    // Render text to surface
    SDL_Surface* surface = TTF_RenderText_Blended(font_->getTTFFont(), text.c_str(), colorToSDL(color));
    if (!surface) {
        std::cerr << "Failed to render text: " << TTF_GetError() << std::endl;
        return;
    }

    // Create texture from surface
    SDL_Renderer* sdlRenderer = static_cast<SDL_Renderer*>(renderer.getBackendContext());
    texture_ = SDL_CreateTextureFromSurface(sdlRenderer, surface);
    if (!texture_) {
        std::cerr << "Failed to create texture from text: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return;
    }

    // Set blend mode for proper alpha rendering
    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);

    width_ = surface->w;
    height_ = surface->h;

    SDL_FreeSurface(surface);
}

} // namespace Engine
