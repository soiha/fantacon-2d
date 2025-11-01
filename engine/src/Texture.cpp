#include "engine/Texture.h"
#include "engine/IRenderer.h"
#include <SDL_image.h>
#include <iostream>

namespace Engine {

Texture::~Texture() {
    if (backendHandle_ && deleter_) {
        deleter_(backendHandle_);
    }
    backendHandle_ = nullptr;
}

Texture::Texture(Texture&& other) noexcept
    : backendHandle_(other.backendHandle_)
    , deleter_(std::move(other.deleter_))
    , width_(other.width_)
    , height_(other.height_) {
    other.backendHandle_ = nullptr;
    other.deleter_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        // Clean up existing resource
        if (backendHandle_ && deleter_) {
            deleter_(backendHandle_);
        }

        // Move from other
        backendHandle_ = other.backendHandle_;
        deleter_ = std::move(other.deleter_);
        width_ = other.width_;
        height_ = other.height_;

        // Reset other
        other.backendHandle_ = nullptr;
        other.deleter_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;
    }
    return *this;
}

void Texture::setHandle(void* handle, DeleterFunc deleter) {
    // Clean up existing resource if any
    if (backendHandle_ && deleter_) {
        deleter_(backendHandle_);
    }

    backendHandle_ = handle;
    deleter_ = deleter;
}

bool Texture::loadFromFile(const std::string& path, IRenderer& renderer) {
    // Clean up existing texture
    if (backendHandle_ && deleter_) {
        deleter_(backendHandle_);
        backendHandle_ = nullptr;
    }

    // Load image using SDL_Image (this is backend-agnostic image loading)
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "Failed to load image " << path << ": " << IMG_GetError() << std::endl;
        return false;
    }

    // Create texture using backend context (SDL_Renderer* for SDL backend)
    // NOTE: This assumes SDL backend for now. For true backend independence,
    // this method should be moved to IRenderer as loadTextureFromFile()
    SDL_Renderer* sdlRenderer = static_cast<SDL_Renderer*>(renderer.getBackendContext());
    SDL_Texture* sdlTexture = SDL_CreateTextureFromSurface(sdlRenderer, surface);
    if (!sdlTexture) {
        std::cerr << "Failed to create texture from " << path << ": " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return false;
    }

    width_ = surface->w;
    height_ = surface->h;
    SDL_FreeSurface(surface);

    // Set the backend handle with SDL-specific deleter
    backendHandle_ = sdlTexture;
    deleter_ = [](void* handle) {
        SDL_DestroyTexture(static_cast<SDL_Texture*>(handle));
    };

    return true;
}

} // namespace Engine
