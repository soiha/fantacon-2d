#pragma once

#include "Types.h"
#include "PixelFont.h"
#include "ILayerAttachable.h"
#include <vector>
#include <array>
#include <memory>
#include <string>

namespace Engine {

// Forward declaration

// Attributed text grid - a character grid where each cell has a glyph and attribute
// Similar to VGA text mode: 16-bit cells with 8-bit glyph + 8-bit attribute index
// Attributes define foreground/background colors (and potentially other flags)
class AttributedTextGrid : public ILayerAttachable {
public:
    // Attribute definition for text cells
    struct Attribute {
        uint8_t foregroundColor;  // Palette index for foreground
        uint8_t backgroundColor;  // Palette index for background

        Attribute(uint8_t fg = 15, uint8_t bg = 0)
            : foregroundColor(fg), backgroundColor(bg) {}
    };

    AttributedTextGrid(PixelFontPtr font, int widthInChars, int heightInChars);
    // Constructor with explicit buffer size
    AttributedTextGrid(PixelFontPtr font, int widthInChars, int heightInChars, int bufferWidth, int bufferHeight);
    ~AttributedTextGrid() = default;

    // Buffer configuration
    void setBufferSize(int width, int height);
    int getBufferWidth() const;
    int getBufferHeight() const;
    std::shared_ptr<IndexedPixelBuffer> getBuffer() const;

    // Grid manipulation - 16-bit cells (8-bit glyph + 8-bit attribute)
    void setCell(int x, int y, uint8_t glyph, uint8_t attribute);
    void setGlyph(int x, int y, uint8_t glyph);
    void setAttribute(int x, int y, uint8_t attribute);
    uint16_t getCell(int x, int y) const;
    uint8_t getGlyph(int x, int y) const;
    uint8_t getAttribute(int x, int y) const;

    // Attribute definitions (256 possible attributes)
    void setAttributeDef(uint8_t index, const Attribute& attr);
    void setAttributeDef(uint8_t index, uint8_t foreground, uint8_t background);
    [[nodiscard]] const Attribute& getAttributeDef(uint8_t index) const noexcept;

    // Text operations
    void print(int x, int y, const std::string& text, uint8_t attribute);
    void print(int x, int y, const std::string& text);  // Uses current attribute

    // Word-wrapped text printing (wraps on spaces, returns number of lines printed)
    // Useful for HUD text, dialog boxes, etc.
    [[nodiscard]] int printWrapped(int x, int y, int maxWidth, const std::string& text, uint8_t attribute);
    [[nodiscard]] int printWrapped(int x, int y, int maxWidth, const std::string& text);  // Uses current attribute

    void clear(uint8_t glyph = ' ', uint8_t attribute = 0);
    void clearLine(int y, uint8_t glyph = ' ', uint8_t attribute = 0);
    void fill(int x, int y, int width, int height, uint8_t glyph, uint8_t attribute);
    void scroll(int dx, int dy);  // Scroll content (positive = right/down)

    // Measurement and hit-testing
    [[nodiscard]] int measureText(const std::string& text) const noexcept;  // Width in characters (max line width)

    // Measure text dimensions for layout - returns {maxWidth, lineCount}
    struct TextMetrics { int maxWidth; int lines; };
    [[nodiscard]] TextMetrics measureTextLines(const std::string& text) const noexcept;

    // Convert pixel coordinates to cell coordinates
    // Uses floor-division semantics for negative coordinates
    // Returns true if resulting cell is within grid bounds, false otherwise
    [[nodiscard]] bool cellFromPixel(int pixelX, int pixelY, int& cellX, int& cellY) const noexcept;

    // Attribute management
    void setCurrentAttribute(uint8_t attribute) noexcept { currentAttribute_ = attribute; }
    [[nodiscard]] uint8_t getCurrentAttribute() const noexcept { return currentAttribute_; }

    // Transparency (background skip optimization)
    // Contract: transparentColorIndex_ == 0xFFFF means "disabled" (no transparency)
    // When enabled, background pixels matching the 8-bit index are not drawn
    // This allows layering multiple grids without overwriting lower layers
    void setTransparentColorIndex(uint8_t index) noexcept { transparentColorIndex_ = index; }
    // NOTE: Only valid if hasTransparency() == true. Otherwise sentinel 0xFFFF is active.
    [[nodiscard]] uint8_t getTransparentColorIndex() const noexcept { return static_cast<uint8_t>(transparentColorIndex_); }
    void clearTransparency() noexcept { transparentColorIndex_ = 0xFFFF; }  // Disable
    [[nodiscard]] bool hasTransparency() const noexcept { return transparentColorIndex_ != 0xFFFF; }

    // Dirty tracking (optimization to skip rendering unchanged grids)
    void markDirty() noexcept { markDirtyAll(); }  // Marks entire grid dirty (coarse)
    void markDirty(int x, int y) noexcept;  // Fine-grained dirty (expands rect)
    void markDirtyAll() noexcept { dirty_ = true; dirtyRectValid_ = false; }
    void markClean() noexcept { dirty_ = false; dirtyRectValid_ = false; }
    [[nodiscard]] bool isDirty() const noexcept { return dirty_; }

    // Dirty rectangle tracking (for partial updates)
    // DirtyRect is inclusive in cell-space: [minX..maxX], [minY..maxY]
    struct DirtyRect { int minX, minY, maxX, maxY; };
    [[nodiscard]] bool hasDirtyRect() const noexcept { return dirtyRectValid_; }
    [[nodiscard]] DirtyRect getDirtyRect() const noexcept { return {dirtyMinX_, dirtyMinY_, dirtyMaxX_, dirtyMaxY_}; }
    void clearDirtyRect() noexcept { dirtyRectValid_ = false; }

    // ILayerAttachable interface
    void render(IRenderer& renderer, const Vec2& layerOffset, float opacity) override;

    // Backward compatibility: render to external buffer
    void renderToBuffer(IndexedPixelBuffer& buffer) const;

    // Transform properties
    void setPosition(const Vec2& pos) { position_ = pos; }
    const Vec2& getPosition() const noexcept { return position_; }

    void setVisible(bool visible) noexcept { visible_ = visible; }
    bool isVisible() const noexcept override { return visible_; }

    // Dimensions
    [[nodiscard]] int getWidth() const noexcept { return width_; }
    [[nodiscard]] int getHeight() const noexcept { return height_; }
    [[nodiscard]] int getPixelWidth() const noexcept;
    [[nodiscard]] int getPixelHeight() const noexcept;

    // Font access
    [[nodiscard]] PixelFontPtr getFont() const noexcept { return font_; }

private:
    // Internal rendering buffer
    std::shared_ptr<IndexedPixelBuffer> buffer_;
    int bufferWidth_ = 0;   // 0 means "use viewport size"
    int bufferHeight_ = 0;  // 0 means "use viewport size"

    // Helper: ensure buffer is created and sized appropriately
    void ensureBuffer(IRenderer& renderer);

    PixelFontPtr font_;
    int width_;   // Grid width in characters
    int height_;  // Grid height in characters
    std::vector<uint16_t> cells_;  // width * height cells (glyph | (attr << 8))
    std::array<Attribute, 256> attributes_;
    uint8_t currentAttribute_ = 0;  // Current attribute for print() without explicit attribute
    uint16_t transparentColorIndex_ = 0xFFFF;  // Transparent palette index (0xFFFF = disabled)
    Vec2 position_{0.0f, 0.0f};
    bool visible_ = true;
    bool dirty_ = true;  // Grid needs re-rendering (default true = render on first frame)

    // Dirty rectangle tracking (min/max touched cell coords for partial updates)
    bool dirtyRectValid_ = false;
    int dirtyMinX_ = 0, dirtyMinY_ = 0;
    int dirtyMaxX_ = 0, dirtyMaxY_ = 0;

    // Helper to pack glyph and attribute into 16-bit cell
    static constexpr inline uint16_t packCell(uint8_t glyph, uint8_t attribute) noexcept {
        return static_cast<uint16_t>(glyph) | (static_cast<uint16_t>(attribute) << 8);
    }

    // Helper to extract glyph from cell
    static constexpr inline uint8_t unpackGlyph(uint16_t cell) noexcept {
        return static_cast<uint8_t>(cell & 0xFF);
    }

    // Helper to extract attribute from cell
    static constexpr inline uint8_t unpackAttribute(uint16_t cell) noexcept {
        return static_cast<uint8_t>((cell >> 8) & 0xFF);
    }
};

using AttributedTextGridPtr = std::shared_ptr<AttributedTextGrid>;

} // namespace Engine
