#include "engine/AttributedTextGrid.h"
#include "engine/IndexedPixelBuffer.h"
#include <algorithm>
#include <cstring>

namespace Engine {

AttributedTextGrid::AttributedTextGrid(PixelFontPtr font, int widthInChars, int heightInChars)
    : font_(font)
    , width_(widthInChars)
    , height_(heightInChars)
    , cells_(widthInChars * heightInChars, packCell(' ', 0)) {

    // Initialize default attributes
    // Attribute 0: white on black (classic terminal default)
    attributes_[0] = Attribute(15, 0);

    // Fill remaining attributes with default values
    for (int i = 1; i < 256; ++i) {
        attributes_[i] = Attribute(15, 0);
    }

    // Create buffer based on grid dimensions and font size
    if (font_) {
        int pixelWidth = width_ * font_->getCharWidth();
        int pixelHeight = height_ * font_->getCharHeight();
        buffer_ = std::make_shared<IndexedPixelBuffer>(pixelWidth, pixelHeight);
    } else {
        // Fallback if no font (should rarely happen)
        buffer_ = std::make_shared<IndexedPixelBuffer>(640, 480);
    }
}

AttributedTextGrid::AttributedTextGrid(PixelFontPtr font, int widthInChars, int heightInChars, int bufferWidth, int bufferHeight)
    : font_(font)
    , width_(widthInChars)
    , height_(heightInChars)
    , bufferWidth_(bufferWidth)
    , bufferHeight_(bufferHeight)
    , cells_(widthInChars * heightInChars, packCell(' ', 0)) {

    // Initialize default attributes
    // Attribute 0: white on black (classic terminal default)
    attributes_[0] = Attribute(15, 0);

    // Fill remaining attributes with default values
    for (int i = 1; i < 256; ++i) {
        attributes_[i] = Attribute(15, 0);
    }

    // Create buffer with specified dimensions or calculated from grid
    int width = bufferWidth_;
    int height = bufferHeight_;

    if (width == 0 || height == 0) {
        // Use grid dimensions
        if (font_) {
            width = width_ * font_->getCharWidth();
            height = height_ * font_->getCharHeight();
        } else {
            // Fallback if no font
            width = 640;
            height = 480;
        }
    }

    buffer_ = std::make_shared<IndexedPixelBuffer>(width, height);
}

void AttributedTextGrid::markDirty(int x, int y) noexcept {
    dirty_ = true;

    // Expand dirty rectangle to include this cell
    if (!dirtyRectValid_) {
        // First dirty cell - initialize rect
        dirtyMinX_ = dirtyMaxX_ = x;
        dirtyMinY_ = dirtyMaxY_ = y;
        dirtyRectValid_ = true;
    } else {
        // Expand existing rect
        dirtyMinX_ = std::min(dirtyMinX_, x);
        dirtyMinY_ = std::min(dirtyMinY_, y);
        dirtyMaxX_ = std::max(dirtyMaxX_, x);
        dirtyMaxY_ = std::max(dirtyMaxY_, y);
    }
}

void AttributedTextGrid::setCell(int x, int y, uint8_t glyph, uint8_t attribute) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return;  // Out of bounds
    }
    cells_[y * width_ + x] = packCell(glyph, attribute);
    markDirty(x, y);
}

void AttributedTextGrid::setGlyph(int x, int y, uint8_t glyph) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return;  // Out of bounds
    }
    uint16_t& cell = cells_[y * width_ + x];
    cell = packCell(glyph, unpackAttribute(cell));
    markDirty(x, y);
}

void AttributedTextGrid::setAttribute(int x, int y, uint8_t attribute) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return;  // Out of bounds
    }
    uint16_t& cell = cells_[y * width_ + x];
    cell = packCell(unpackGlyph(cell), attribute);
    markDirty(x, y);
}

uint16_t AttributedTextGrid::getCell(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return 0;  // Out of bounds
    }
    return cells_[y * width_ + x];
}

uint8_t AttributedTextGrid::getGlyph(int x, int y) const {
    return unpackGlyph(getCell(x, y));
}

uint8_t AttributedTextGrid::getAttribute(int x, int y) const {
    return unpackAttribute(getCell(x, y));
}

void AttributedTextGrid::setAttributeDef(uint8_t index, const Attribute& attr) {
    attributes_[index] = attr;
}

void AttributedTextGrid::setAttributeDef(uint8_t index, uint8_t foreground, uint8_t background) {
    attributes_[index] = Attribute(foreground, background);
}

const AttributedTextGrid::Attribute& AttributedTextGrid::getAttributeDef(uint8_t index) const noexcept {
    return attributes_[index];
}

void AttributedTextGrid::print(int x, int y, const std::string& text, uint8_t attribute) {
    int cursorX = x;
    int cursorY = y;

    for (char c : text) {
        // Stop if we've walked off the bottom of the grid
        if (cursorY >= height_) {
            break;
        }

        // Handle newline
        if (c == '\n') {
            cursorX = x;
            cursorY++;
            if (cursorY >= height_) {
                break;  // Short-circuit: newline pushed us past bottom
            }
            continue;
        }

        // Set the cell
        if (cursorX >= 0 && cursorX < width_ && cursorY >= 0 && cursorY < height_) {
            setCell(cursorX, cursorY, static_cast<uint8_t>(c), attribute);
        }

        cursorX++;
    }
}

void AttributedTextGrid::print(int x, int y, const std::string& text) {
    print(x, y, text, currentAttribute_);
}

int AttributedTextGrid::printWrapped(int x, int y, int maxWidth, const std::string& text, uint8_t attribute) {
    if (maxWidth <= 0) {
        return 0;  // Invalid width
    }

    int cursorX = 0;  // Relative to x
    int cursorY = 0;  // Relative to y
    int linesPrinted = 0;
    std::string currentWord;

    auto flushWord = [&]() {
        if (currentWord.empty()) return;

        // Check if word fits on current line
        int wordLen = static_cast<int>(currentWord.size());
        if (cursorX + wordLen > maxWidth) {
            // Word doesn't fit, wrap to next line
            cursorX = 0;
            cursorY++;
            linesPrinted++;

            // Stop if we've walked off the bottom
            if (y + cursorY >= height_) {
                currentWord.clear();
                return;
            }
        }

        // Print the word
        for (char c : currentWord) {
            if (x + cursorX >= 0 && x + cursorX < width_ && y + cursorY >= 0 && y + cursorY < height_) {
                setCell(x + cursorX, y + cursorY, static_cast<uint8_t>(c), attribute);
            }
            cursorX++;

            // Check if we've reached the end of the line
            if (cursorX >= maxWidth) {
                cursorX = 0;
                cursorY++;
                linesPrinted++;

                // Stop if we've walked off the bottom
                if (y + cursorY >= height_) {
                    currentWord.clear();
                    return;
                }
            }
        }

        currentWord.clear();
    };

    for (char c : text) {
        // Handle explicit newlines
        if (c == '\n') {
            flushWord();
            cursorX = 0;
            cursorY++;
            linesPrinted++;

            // Stop if we've walked off the bottom
            if (y + cursorY >= height_) {
                break;
            }
            continue;
        }

        // Handle spaces - flush current word and add space
        if (c == ' ') {
            flushWord();

            // Add space if there's room on current line
            if (cursorX < maxWidth) {
                if (x + cursorX >= 0 && x + cursorX < width_ && y + cursorY >= 0 && y + cursorY < height_) {
                    setCell(x + cursorX, y + cursorY, ' ', attribute);
                }
                cursorX++;
            } else {
                // Space at end of line, wrap to next line
                cursorX = 0;
                cursorY++;
                linesPrinted++;

                // Stop if we've walked off the bottom
                if (y + cursorY >= height_) {
                    break;
                }
            }
            continue;
        }

        // Regular character - add to current word
        currentWord += c;
    }

    // Flush any remaining word
    flushWord();

    // Count the final line if we printed anything
    if (cursorX > 0 || linesPrinted == 0) {
        linesPrinted++;
    }

    return linesPrinted;
}

int AttributedTextGrid::printWrapped(int x, int y, int maxWidth, const std::string& text) {
    return printWrapped(x, y, maxWidth, text, currentAttribute_);
}

void AttributedTextGrid::clear(uint8_t glyph, uint8_t attribute) {
    std::fill(cells_.begin(), cells_.end(), packCell(glyph, attribute));
    markDirty();
}

void AttributedTextGrid::fill(int x, int y, int width, int height, uint8_t glyph, uint8_t attribute) {
    for (int dy = 0; dy < height; ++dy) {
        for (int dx = 0; dx < width; ++dx) {
            setCell(x + dx, y + dy, glyph, attribute);
        }
    }
}

void AttributedTextGrid::clearLine(int y, uint8_t glyph, uint8_t attribute) {
    if (y < 0 || y >= height_) {
        return;
    }
    fill(0, y, width_, 1, glyph, attribute);
}

void AttributedTextGrid::scroll(int dx, int dy) {
    if (dx == 0 && dy == 0) {
        return;  // No scroll
    }

    // Fast path for pure vertical scroll (common in terminals)
    if (dx == 0 && dy != 0) {
        std::vector<uint16_t> newCells(cells_.size(), packCell(' ', 0));

        // Calculate valid row range
        int srcStartY = (dy > 0) ? dy : 0;
        int srcEndY = (dy > 0) ? height_ : height_ + dy;
        int dstStartY = (dy > 0) ? 0 : -dy;

        // Bulk copy contiguous rows (guard against empty span)
        if (srcStartY < srcEndY && width_ > 0) {
            for (int y = srcStartY; y < srcEndY; ++y) {
                int dstY = y - dy;
                std::memcpy(&newCells[dstY * width_], &cells_[y * width_], width_ * sizeof(uint16_t));
            }
        }

        cells_ = std::move(newCells);
        markDirty();
        return;
    }

    // Fast path for pure horizontal scroll
    if (dy == 0 && dx != 0) {
        std::vector<uint16_t> newCells(cells_.size(), packCell(' ', 0));

        // Calculate valid column range
        int srcStartX = (dx > 0) ? dx : 0;
        int srcEndX = (dx > 0) ? width_ : width_ + dx;
        int copyWidth = srcEndX - srcStartX;

        // Copy row segments (shifted horizontally) - guard against zero-length span
        if (copyWidth > 0) {
            for (int y = 0; y < height_; ++y) {
                int srcOffset = y * width_ + srcStartX;
                int dstOffset = y * width_ + (srcStartX - dx);
                std::memcpy(&newCells[dstOffset], &cells_[srcOffset], copyWidth * sizeof(uint16_t));
            }
        }

        cells_ = std::move(newCells);
        markDirty();
        return;
    }

    // General case: diagonal scroll (slower double loop)
    std::vector<uint16_t> newCells(cells_.size(), packCell(' ', 0));

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            int srcX = x - dx;
            int srcY = y - dy;

            // If source is in bounds, copy it
            if (srcX >= 0 && srcX < width_ && srcY >= 0 && srcY < height_) {
                newCells[y * width_ + x] = cells_[srcY * width_ + srcX];
            }
            // else: newCells already has blank (space, attr 0)
        }
    }

    cells_ = std::move(newCells);
    markDirty();
}

int AttributedTextGrid::measureText(const std::string& text) const noexcept {
    int maxWidth = 0;
    int currentWidth = 0;

    for (char c : text) {
        if (c == '\n') {
            maxWidth = std::max(maxWidth, currentWidth);
            currentWidth = 0;
        } else {
            currentWidth++;
        }
    }

    return std::max(maxWidth, currentWidth);
}

AttributedTextGrid::TextMetrics AttributedTextGrid::measureTextLines(const std::string& text) const noexcept {
    int maxWidth = 0;
    int currentWidth = 0;
    int lines = 1;  // Start with 1 line (empty string = 1 line)

    for (char c : text) {
        if (c == '\n') {
            maxWidth = std::max(maxWidth, currentWidth);
            currentWidth = 0;
            lines++;
        } else {
            currentWidth++;
        }
    }

    maxWidth = std::max(maxWidth, currentWidth);
    return {maxWidth, lines};
}

void AttributedTextGrid::setBufferSize(int width, int height) {
    bufferWidth_ = width;
    bufferHeight_ = height;
    buffer_.reset();  // Force recreation on next render
}

int AttributedTextGrid::getBufferWidth() const {
    if (bufferWidth_ > 0) {
        return bufferWidth_;
    }
    return buffer_ ? buffer_->getWidth() : 0;
}

int AttributedTextGrid::getBufferHeight() const {
    if (bufferHeight_ > 0) {
        return bufferHeight_;
    }
    return buffer_ ? buffer_->getHeight() : 0;
}

std::shared_ptr<IndexedPixelBuffer> AttributedTextGrid::getBuffer() const {
    return buffer_;
}

void AttributedTextGrid::ensureBuffer(IRenderer& renderer) {
    int width = bufferWidth_;
    int height = bufferHeight_;

    // 0 means use viewport size
    if (width == 0) {
        width = renderer.getViewportWidth();
    }
    if (height == 0) {
        height = renderer.getViewportHeight();
    }

    // Create buffer if it doesn't exist or size changed
    if (!buffer_ || buffer_->getWidth() != width || buffer_->getHeight() != height) {
        // Preserve palette if buffer exists
        std::array<Color, 256> palette;
        bool hasPalette = false;
        if (buffer_) {
            palette = buffer_->getPalette();
            hasPalette = true;
        }

        buffer_ = std::make_shared<IndexedPixelBuffer>(width, height);

        // Restore palette if we had one
        if (hasPalette) {
            buffer_->setPalette(palette);
        }
    }
}

bool AttributedTextGrid::cellFromPixel(int pixelX, int pixelY, int& cellX, int& cellY) const noexcept {
    if (!font_) {
        return false;
    }

    // Adjust for grid position
    int localX = pixelX - static_cast<int>(position_.x);
    int localY = pixelY - static_cast<int>(position_.y);

    int charWidth = font_->getCharWidth();
    int charHeight = font_->getCharHeight();

    cellX = localX / charWidth;
    cellY = localY / charHeight;

    // Return true if cell is in bounds
    return (cellX >= 0 && cellX < width_ && cellY >= 0 && cellY < height_);
}

void AttributedTextGrid::render(IRenderer& renderer, const Vec2& layerOffset, float opacity) {
    if (!visible_) {
        return;
    }

    // Ensure internal buffer is ready
    ensureBuffer(renderer);

    // Clear buffer to transparent
    buffer_->clear(0);

    // Render text grid to internal buffer
    renderToBuffer(*buffer_);

    // Render buffer to screen via renderer
    renderer.renderIndexedPixelBuffer(*buffer_, layerOffset, opacity);
}

void AttributedTextGrid::renderToBuffer(IndexedPixelBuffer& buffer) const {
    if (!visible_ || !font_) {
        return;
    }

    int charWidth = font_->getCharWidth();
    int charHeight = font_->getCharHeight();

    // Get buffer dimensions for culling
    int bufferWidth = buffer.getWidth();
    int bufferHeight = buffer.getHeight();

    // Render each cell
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            uint16_t cell = cells_[y * width_ + x];
            uint8_t glyph = unpackGlyph(cell);
            uint8_t attrIndex = unpackAttribute(cell);
            // Safe: attributes_ is std::array<Attribute, 256>, attrIndex is uint8_t (0-255)
            const Attribute& attr = attributes_[attrIndex];

            // Calculate pixel position
            int pixelX = static_cast<int>(position_.x) + x * charWidth;
            int pixelY = static_cast<int>(position_.y) + y * charHeight;

            // Character-level culling: skip characters that are COMPLETELY off-screen
            // Allow partially visible characters to be clipped at pixel boundaries for smooth scrolling
            if (pixelX + charWidth <= 0 || pixelX >= bufferWidth ||
                pixelY + charHeight <= 0 || pixelY >= bufferHeight) {
                continue;  // Character completely off-screen
            }

            // Pixel-level clamping: only render visible pixels for efficiency
            // This avoids calling setPixel() for pixels we know are off-screen
            int startX = std::max(0, -pixelX);
            int startY = std::max(0, -pixelY);
            int endX = std::min(charWidth, bufferWidth - pixelX);
            int endY = std::min(charHeight, bufferHeight - pixelY);

            // Early-out if clamped span is empty (safety guard)
            if (startX >= endX || startY >= endY) {
                continue;
            }

            // Check if background is transparent
            bool transparentBg = (transparentColorIndex_ != 0xFFFF &&
                                 attr.backgroundColor == static_cast<uint8_t>(transparentColorIndex_));

            // Draw background first as solid rectangle (only if not transparent)
            // Use fillRect for better performance (direct memory access vs individual setPixel calls)
            if (!transparentBg) {
                int bgWidth = endX - startX;
                int bgHeight = endY - startY;
                buffer.fillRect(pixelX + startX, pixelY + startY, bgWidth, bgHeight, attr.backgroundColor);
            }

            // Skip rendering foreground for space characters
            // If space + transparent bg, we skipped everything above - fast path!
            if (glyph == ' ') {
                continue;  // Space is just background (or nothing if transparent)
            }

            // Get glyph bitmap
            const auto& glyphBitmap = font_->getGlyph(glyph);
            if (glyphBitmap.empty()) {
                continue;  // No glyph, background already drawn
            }

            // Draw foreground (glyph) ONLY where glyph pixels are (only visible portion)
            // Use same logic as IndexedPixelBuffer::drawText()
            for (int dy = startY; dy < endY; ++dy) {
                for (int dx = startX; dx < endX; ++dx) {
                    const Color& pixel = glyphBitmap[dy * charWidth + dx];

                    // For bitmap fonts: check if pixel is dark (below threshold) = glyph pixel
                    // This handles inverted fonts where black = glyph, white = background
                    if (pixel.r < 127) {
                        buffer.setPixel(pixelX + dx, pixelY + dy, attr.foregroundColor);
                    }
                    // Else: leave the background we already drew
                }
            }
        }
    }
}

int AttributedTextGrid::getPixelWidth() const noexcept {
    if (!font_) return 0;
    return width_ * font_->getCharWidth();
}

int AttributedTextGrid::getPixelHeight() const noexcept {
    if (!font_) return 0;
    return height_ * font_->getCharHeight();
}

} // namespace Engine
