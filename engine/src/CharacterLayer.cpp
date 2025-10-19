#include "engine/CharacterLayer.h"
#include "engine/IndexedPixelBuffer.h"
#include <algorithm>

namespace Engine {

CharacterLayer::CharacterLayer(int cols, int rows, const PixelFontPtr& font)
    : cols_(cols)
    , rows_(rows)
    , font_(font)
    , characters_(cols * rows, ' ')
    , colors_(cols * rows, 1) {
}

void CharacterLayer::print(const std::string& text) {
    for (char c : text) {
        print(c);
    }
}

void CharacterLayer::print(char c) {
    // Handle special characters
    if (c == '\n') {
        cursorCol_ = 0;
        cursorRow_++;
        if (cursorRow_ >= rows_) {
            scrollUp(1);
            cursorRow_ = rows_ - 1;
        }
        return;
    }

    if (c == '\r') {
        cursorCol_ = 0;
        return;
    }

    if (c == '\t') {
        // Tab to next 4-column boundary
        int nextTab = ((cursorCol_ / 4) + 1) * 4;
        cursorCol_ = std::min(nextTab, cols_ - 1);
        return;
    }

    // Print character at cursor position
    if (cursorCol_ >= 0 && cursorCol_ < cols_ && cursorRow_ >= 0 && cursorRow_ < rows_) {
        int index = cursorRow_ * cols_ + cursorCol_;
        characters_[index] = c;
        colors_[index] = currentColor_;
    }

    // Advance cursor
    advanceCursor();
}

void CharacterLayer::advanceCursor() {
    cursorCol_++;
    if (cursorCol_ >= cols_) {
        cursorCol_ = 0;
        cursorRow_++;
        if (cursorRow_ >= rows_) {
            scrollUp(1);
            cursorRow_ = rows_ - 1;
        }
    }
}

void CharacterLayer::clear() {
    std::fill(characters_.begin(), characters_.end(), ' ');
    std::fill(colors_.begin(), colors_.end(), currentColor_);
    home();
}

void CharacterLayer::setCursor(int col, int row) {
    cursorCol_ = std::clamp(col, 0, cols_ - 1);
    cursorRow_ = std::clamp(row, 0, rows_ - 1);
}

void CharacterLayer::getCursor(int& col, int& row) const {
    col = cursorCol_;
    row = cursorRow_;
}

void CharacterLayer::moveCursor(int deltaCol, int deltaRow) {
    setCursor(cursorCol_ + deltaCol, cursorRow_ + deltaRow);
}

void CharacterLayer::setChar(int col, int row, char c) {
    if (col >= 0 && col < cols_ && row >= 0 && row < rows_) {
        int index = row * cols_ + col;
        characters_[index] = c;
    }
}

char CharacterLayer::getChar(int col, int row) const {
    if (col >= 0 && col < cols_ && row >= 0 && row < rows_) {
        int index = row * cols_ + col;
        return characters_[index];
    }
    return ' ';
}

void CharacterLayer::setCellColor(int col, int row, uint8_t colorIndex) {
    if (col >= 0 && col < cols_ && row >= 0 && row < rows_) {
        int index = row * cols_ + col;
        colors_[index] = colorIndex;
    }
}

uint8_t CharacterLayer::getCellColor(int col, int row) const {
    if (col >= 0 && col < cols_ && row >= 0 && row < rows_) {
        int index = row * cols_ + col;
        return colors_[index];
    }
    return 1;
}

void CharacterLayer::scrollUp(int lines) {
    if (lines <= 0 || lines >= rows_) {
        clear();
        return;
    }

    // Move all rows up
    for (int row = 0; row < rows_ - lines; ++row) {
        for (int col = 0; col < cols_; ++col) {
            int srcIndex = (row + lines) * cols_ + col;
            int dstIndex = row * cols_ + col;
            characters_[dstIndex] = characters_[srcIndex];
            colors_[dstIndex] = colors_[srcIndex];
        }
    }

    // Clear bottom lines
    for (int row = rows_ - lines; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            int index = row * cols_ + col;
            characters_[index] = ' ';
            colors_[index] = currentColor_;
        }
    }
}

void CharacterLayer::scrollDown(int lines) {
    if (lines <= 0 || lines >= rows_) {
        clear();
        return;
    }

    // Move all rows down
    for (int row = rows_ - 1; row >= lines; --row) {
        for (int col = 0; col < cols_; ++col) {
            int srcIndex = (row - lines) * cols_ + col;
            int dstIndex = row * cols_ + col;
            characters_[dstIndex] = characters_[srcIndex];
            colors_[dstIndex] = colors_[srcIndex];
        }
    }

    // Clear top lines
    for (int row = 0; row < lines; ++row) {
        for (int col = 0; col < cols_; ++col) {
            int index = row * cols_ + col;
            characters_[index] = ' ';
            colors_[index] = currentColor_;
        }
    }
}

void CharacterLayer::render(IndexedPixelBuffer& buffer) {
    if (!font_) {
        return;
    }

    int charWidth = font_->getCharWidth();
    int charHeight = font_->getCharHeight();

    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            int index = row * cols_ + col;
            char c = characters_[index];
            uint8_t colorIndex = colors_[index];

            // Skip spaces for efficiency
            if (c == ' ') {
                continue;
            }

            // Calculate pixel position
            int x = col * charWidth;
            int y = row * charHeight;

            // Draw character
            std::string singleChar(1, c);
            buffer.drawText(font_, singleChar, x, y, colorIndex);
        }
    }
}

} // namespace Engine
