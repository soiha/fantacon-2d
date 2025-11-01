#pragma once

#include <string>
#include <memory>
#include <functional>

namespace Engine {

class IRenderer;

class Texture {
public:
    // Custom deleter function type for backend-specific cleanup
    using DeleterFunc = std::function<void(void*)>;

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
    void* getHandle() const { return backendHandle_; }  // Backend-agnostic handle getter
    bool isValid() const { return backendHandle_ != nullptr; }

    // For programmatically created textures (e.g., streaming textures)
    // The deleter function is called when the texture is destroyed
    void setHandle(void* handle, DeleterFunc deleter = nullptr);
    void setDimensions(int width, int height) { width_ = width; height_ = height; }

    // Legacy SDL-specific getter (for backward compatibility during transition)
    // This will be removed once all code uses getHandle()
    void* getSDLTexture() const { return backendHandle_; }

private:
    void* backendHandle_ = nullptr;  // Backend-specific texture handle (SDL_Texture*, GLuint, VkImage, etc.)
    DeleterFunc deleter_;             // Backend-specific cleanup function
    int width_ = 0;
    int height_ = 0;
};

using TexturePtr = std::shared_ptr<Texture>;

} // namespace Engine
