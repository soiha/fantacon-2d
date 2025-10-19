#pragma once

#include <SDL_ttf.h>
#include <string>
#include <memory>

namespace Engine {

// Font resource - represents a loaded TTF font at a specific size
// Separate from Text (which is rendered text using a Font)
class Font {
public:
    Font() = default;
    ~Font();

    // Non-copyable but movable
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&& other) noexcept;
    Font& operator=(Font&& other) noexcept;

    // Load font from file
    bool loadFromFile(const std::string& path, int size);

    // Get underlying TTF_Font pointer
    TTF_Font* getTTFFont() const { return font_; }
    bool isValid() const { return font_ != nullptr; }

    // Font properties
    int getSize() const { return size_; }
    const std::string& getPath() const { return path_; }

private:
    void cleanup();

    TTF_Font* font_ = nullptr;
    int size_ = 0;
    std::string path_;
};

using FontPtr = std::shared_ptr<Font>;

} // namespace Engine
