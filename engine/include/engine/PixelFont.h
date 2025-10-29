#pragma once

#include "Types.h"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

namespace Engine {

// Bitmap font for old-school text rendering in pixel buffers
// Loads a texture containing ASCII glyphs arranged in a grid
class PixelFont {
public:
    PixelFont() = default;
    ~PixelFont() = default;

    // Load font from image file with grid layout
    // charWidth, charHeight: dimensions of each character cell
    // charsPerRow: number of characters per row in the texture
    // charMap: string mapping characters to glyphs (e.g., "@ABCDEFG..." for C64)
    //          If empty, uses sequential ASCII starting from firstChar
    // firstChar: ASCII code of first character (used only if charMap is empty)
    // charCount: total number of characters to load
    bool loadFromFile(const std::string& path,
                      int charWidth,
                      int charHeight,
                      int charsPerRow = 16,
                      const std::string& charMap = "",
                      int firstChar = 32,
                      int charCount = 96);

    // Load font from binary file (raw glyph data)
    // Binary format: sequential glyphs, each glyph is charWidth*charHeight bytes (0 or 1)
    // charMap: string mapping characters to glyph indices (e.g., "@ABCDEFG..." for C64)
    //          If empty, uses sequential ASCII starting from firstChar
    // firstChar: ASCII code of first character (used only if charMap is empty)
    bool loadFromBinary(const std::string& path,
                        int charWidth,
                        int charHeight,
                        const std::string& charMap = "",
                        int firstChar = 0);

    // Get glyph pixel data for a character (RGBA format)
    // Returns empty vector if character doesn't exist
    const std::vector<Color>& getGlyph(char c) const;

    // Get glyph as 1-bit packed bitmask (for monochrome fonts)
    // Returns empty vector if character doesn't exist or font is not monochrome
    // Each byte contains 8 pixels (MSB first), padded to byte boundary per row
    const std::vector<uint8_t>& getGlyphBitmask(char c) const;

    // Check if this font has optimized bitmasks available
    bool hasBitmasks() const { return !glyphBitmasks_.empty(); }

    // Check if character exists in font
    bool hasGlyph(char c) const;

    // Get the character map
    const std::string& getCharMap() const { return charMap_; }

    // Get character dimensions
    int getCharWidth() const { return charWidth_; }
    int getCharHeight() const { return charHeight_; }

    // Get first character code
    int getFirstChar() const { return firstChar_; }

    // Get character count
    int getCharCount() const { return static_cast<int>(glyphs_.size()); }

private:
    int charWidth_ = 0;
    int charHeight_ = 0;
    int charsPerRow_ = 16;
    int firstChar_ = 32;
    std::string charMap_;  // Character map for custom encodings
    std::unordered_map<char, std::vector<Color>> glyphs_;
    std::vector<Color> emptyGlyph_;  // Returned when glyph not found

    // Optimized 1-bit bitmask representation (for monochrome fonts)
    std::unordered_map<char, std::vector<uint8_t>> glyphBitmasks_;
    std::vector<uint8_t> emptyBitmask_;  // Returned when bitmask not found
};

using PixelFontPtr = std::shared_ptr<PixelFont>;

} // namespace Engine
