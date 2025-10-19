#include "engine/Font.h"
#include <iostream>

namespace Engine {

Font::~Font() {
    cleanup();
}

Font::Font(Font&& other) noexcept
    : font_(other.font_)
    , size_(other.size_)
    , path_(std::move(other.path_)) {
    other.font_ = nullptr;
    other.size_ = 0;
}

Font& Font::operator=(Font&& other) noexcept {
    if (this != &other) {
        cleanup();
        font_ = other.font_;
        size_ = other.size_;
        path_ = std::move(other.path_);
        other.font_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

void Font::cleanup() {
    if (font_) {
        // Only close font if TTF is still initialized
        if (TTF_WasInit()) {
            TTF_CloseFont(font_);
        }
        font_ = nullptr;
    }
}

bool Font::loadFromFile(const std::string& path, int size) {
    cleanup();

    if (!TTF_WasInit()) {
        std::cerr << "Cannot load font: SDL_ttf not initialized!" << std::endl;
        return false;
    }

    font_ = TTF_OpenFont(path.c_str(), size);
    if (!font_) {
        std::cerr << "Failed to load font " << path << " at size " << size
                  << ": " << TTF_GetError() << std::endl;
        return false;
    }

    size_ = size;
    path_ = path;
    return true;
}

} // namespace Engine
