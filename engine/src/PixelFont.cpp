#include "engine/PixelFont.h"
#include <SDL_image.h>
#include <iostream>
#include <fstream>

namespace Engine {

bool PixelFont::loadFromFile(const std::string& path,
                              int charWidth,
                              int charHeight,
                              int charsPerRow,
                              const std::string& charMap,
                              int firstChar,
                              int charCount) {
    // Load image using SDL_image
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "Failed to load pixel font: " << path << " - " << IMG_GetError() << std::endl;
        return false;
    }

    // Convert to RGBA8888 format
    SDL_Surface* convertedSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);

    if (!convertedSurface) {
        std::cerr << "Failed to convert pixel font surface format" << std::endl;
        return false;
    }

    SDL_LockSurface(convertedSurface);
    Uint32* pixels = static_cast<Uint32*>(convertedSurface->pixels);

    // Store font parameters
    charWidth_ = charWidth;
    charHeight_ = charHeight;
    charsPerRow_ = charsPerRow;
    firstChar_ = firstChar;
    charMap_ = charMap;

    // Extract each character glyph from the grid
    for (int i = 0; i < charCount; ++i) {
        // Determine which character this glyph represents
        char charCode;
        if (!charMap.empty() && i < static_cast<int>(charMap.size())) {
            // Use character from the provided map
            charCode = charMap[i];
        } else {
            // Use sequential ASCII starting from firstChar
            charCode = static_cast<char>(firstChar + i);
        }

        // Calculate position in grid using floating point for accurate alignment
        // This handles cases where the image size doesn't divide evenly
        float actualCharWidth = static_cast<float>(convertedSurface->w) / charsPerRow;
        float actualCharHeight = static_cast<float>(convertedSurface->h) / charsPerRow;  // Assuming square grid

        float gridXFloat = (i % charsPerRow) * actualCharWidth;
        float gridYFloat = (i / charsPerRow) * actualCharHeight;

        int gridX = static_cast<int>(std::round(gridXFloat));
        int gridY = static_cast<int>(std::round(gridYFloat));

        // Check if this character is within the image bounds
        if (gridX + charWidth > convertedSurface->w || gridY + charHeight > convertedSurface->h) {
            std::cerr << "Warning: Character " << static_cast<int>(charCode)
                      << " at grid position (" << gridX << "," << gridY
                      << ") exceeds image bounds" << std::endl;
            break;
        }

        // Extract character pixels
        // Calculate offset if the actual cell size is larger than requested char size
        // (e.g., 8x8 glyphs in 16x16 cells)
        int offsetX = (static_cast<int>(actualCharWidth) - charWidth) / 2;
        int offsetY = (static_cast<int>(actualCharHeight) - charHeight) / 2;

        std::vector<Color> glyph(charWidth * charHeight);
        for (int y = 0; y < charHeight; ++y) {
            for (int x = 0; x < charWidth; ++x) {
                int srcX = gridX + offsetX + x;
                int srcY = gridY + offsetY + y;

                // Bounds check
                if (srcX < 0 || srcX >= convertedSurface->w || srcY < 0 || srcY >= convertedSurface->h) {
                    glyph[y * charWidth + x] = Color{0, 0, 0, 0};
                    continue;
                }

                Uint32 pixel = pixels[srcY * convertedSurface->w + srcX];

                // Use SDL_GetRGBA to properly extract colors (handles endianness)
                Color color;
                SDL_GetRGBA(pixel, convertedSurface->format, &color.r, &color.g, &color.b, &color.a);

                glyph[y * charWidth + x] = color;
            }
        }

        glyphs_[charCode] = glyph;
    }

    SDL_UnlockSurface(convertedSurface);
    SDL_FreeSurface(convertedSurface);

    std::cout << "Loaded pixel font '" << path << "' with " << glyphs_.size()
              << " characters (" << charWidth << "x" << charHeight << ")" << std::endl;

    return true;
}

bool PixelFont::loadFromBinary(const std::string& path,
                                int charWidth,
                                int charHeight,
                                const std::string& charMap,
                                int firstChar) {
    // Open binary file
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "Failed to load binary font: " << path << std::endl;
        return false;
    }

    // Read all data
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
    in.close();

    // Store font parameters
    charWidth_ = charWidth;
    charHeight_ = charHeight;
    firstChar_ = firstChar;
    charMap_ = charMap;

    int bytesPerGlyph = charWidth * charHeight;
    int numGlyphs = data.size() / bytesPerGlyph;

    // Extract each glyph
    for (int i = 0; i < numGlyphs; ++i) {
        // Determine which character this glyph represents
        char charCode;
        if (!charMap.empty() && i < static_cast<int>(charMap.size())) {
            // Use character from the provided map
            charCode = charMap[i];
        } else {
            // Use sequential starting from firstChar
            charCode = static_cast<char>(firstChar + i);
        }

        // Extract glyph pixels
        std::vector<Color> glyph(bytesPerGlyph);
        int offset = i * bytesPerGlyph;

        for (int j = 0; j < bytesPerGlyph; ++j) {
            uint8_t pixel = data[offset + j];
            // Convert 0/1 to black/white
            uint8_t value = pixel ? 255 : 0;
            glyph[j] = Color{value, value, value, 255};
        }

        glyphs_[charCode] = glyph;
    }

    std::cout << "Loaded binary font '" << path << "' with " << glyphs_.size()
              << " characters (" << charWidth << "x" << charHeight << ")" << std::endl;

    return true;
}

const std::vector<Color>& PixelFont::getGlyph(char c) const {
    auto it = glyphs_.find(c);
    if (it != glyphs_.end()) {
        return it->second;
    }
    return emptyGlyph_;
}

bool PixelFont::hasGlyph(char c) const {
    return glyphs_.find(c) != glyphs_.end();
}

} // namespace Engine
