#include "engine/Texture.h"
#include "engine/IRenderer.h"
#include <SDL_image.h>
#include <iostream>

namespace Engine {

Texture::~Texture() {
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
}

Texture::Texture(Texture&& other) noexcept
    : texture_(other.texture_)
    , width_(other.width_)
    , height_(other.height_) {
    other.texture_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (texture_) {
            SDL_DestroyTexture(texture_);
        }
        texture_ = other.texture_;
        width_ = other.width_;
        height_ = other.height_;
        other.texture_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;
    }
    return *this;
}

bool Texture::loadFromFile(const std::string& path, IRenderer& renderer) {
    // Clean up existing texture
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }

    // Load image
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "Failed to load image " << path << ": " << IMG_GetError() << std::endl;
        return false;
    }

    // Create texture using backend context (SDL_Renderer* for SDL backend)
    SDL_Renderer* sdlRenderer = static_cast<SDL_Renderer*>(renderer.getBackendContext());
    texture_ = SDL_CreateTextureFromSurface(sdlRenderer, surface);
    if (!texture_) {
        std::cerr << "Failed to create texture from " << path << ": " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return false;
    }

    width_ = surface->w;
    height_ = surface->h;

    SDL_FreeSurface(surface);
    return true;
}

} // namespace Engine
