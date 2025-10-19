#pragma once

#include "Types.h"
#include "PixelFont.h"
#include "IndexedPixelBuffer.h"
#include <vector>
#include <string>
#include <memory>

namespace Engine {

class IRenderer;

// Text mode character layer
// Similar to classic text mode displays (VGA text mode, C64 screen RAM, etc.)
// Stores a grid of characters and renders them using a bitmap font
class CharacterLayer {
public:
    CharacterLayer(int cols, int rows, const PixelFontPtr& font);
    ~CharacterLayer() = default;

    // Print text at current cursor position
    void print(const std::string& text);
    void print(char c);

    // Clear screen and reset cursor
    void clear();

    // Cursor control
    void setCursor(int col, int row);
    void getCursor(int& col, int& row) const;
    void moveCursor(int deltaCol, int deltaRow);
    void home() { cursorCol_ = 0; cursorRow_ = 0; }

    // Direct character access
    void setChar(int col, int row, char c);
    char getChar(int col, int row) const;

    // Color control
    void setColor(uint8_t colorIndex) { currentColor_ = colorIndex; }
    uint8_t getColor() const { return currentColor_; }

    // Set color for specific cell
    void setCellColor(int col, int row, uint8_t colorIndex);
    uint8_t getCellColor(int col, int row) const;

    // Scrolling
    void scrollUp(int lines = 1);
    void scrollDown(int lines = 1);

    // Rendering - renders to an IndexedPixelBuffer
    void render(IndexedPixelBuffer& buffer);

    // Dimensions
    int getColumns() const { return cols_; }
    int getRows() const { return rows_; }

    // Font
    const PixelFontPtr& getFont() const { return font_; }

private:
    int cols_;                      // Number of character columns
    int rows_;                      // Number of character rows
    PixelFontPtr font_;             // Bitmap font to use

    std::vector<char> characters_;  // Character grid (cols * rows)
    std::vector<uint8_t> colors_;   // Color grid (cols * rows)

    int cursorCol_ = 0;             // Cursor column (0-based)
    int cursorRow_ = 0;             // Cursor row (0-based)
    uint8_t currentColor_ = 1;      // Current drawing color

    // Helper to advance cursor
    void advanceCursor();
};

using CharacterLayerPtr = std::shared_ptr<CharacterLayer>;

} // namespace Engine
