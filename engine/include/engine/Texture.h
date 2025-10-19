#pragma once

#include <SDL.h>
#include <string>
#include <memory>

namespace Engine {

class IRenderer;

class Texture {
public:
    Texture() = default;
    ~Texture();

    // Non-copyable but movable
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool loadFromFile(const std::string& path, IRenderer& renderer);

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    SDL_Texture* getSDLTexture() const { return texture_; }
    void* getHandle() const { return texture_; }  // Generic handle getter
    bool isValid() const { return texture_ != nullptr; }

    // For programmatically created textures (e.g., streaming textures)
    void setHandle(void* handle) { texture_ = static_cast<SDL_Texture*>(handle); }
    void setDimensions(int width, int height) { width_ = width; height_ = height; }

private:
    SDL_Texture* texture_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

using TexturePtr = std::shared_ptr<Texture>;

} // namespace Engine
