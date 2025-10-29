#pragma once

#include "Types.h"
#include "Texture.h"
#include "Palette.h"
#include "PixelFont.h"
#include "ILayerAttachable.h"
#include <vector>
#include <memory>
#include <array>
#include <string>

namespace Engine {

class IRenderer;

// A bitmap buffer using indexed color (256-color palette)
// Perfect for retro graphics, palette effects, and demoscene tricks
// Each pixel is a single byte indexing into a 256-color palette
class IndexedPixelBuffer : public ILayerAttachable {
public:
    IndexedPixelBuffer(int width, int height);
    ~IndexedPixelBuffer() = default;

    // Pixel manipulation (coordinates are clamped to buffer size)
    void setPixel(int x, int y, uint8_t paletteIndex);
    uint8_t getPixel(int x, int y) const;

    // Drawing primitives
    void drawLine(int x0, int y0, int x1, int y1, uint8_t paletteIndex);
    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t paletteIndex);
    void fillRect(int x, int y, int width, int height, uint8_t paletteIndex);  // Optimized rectangle fill

    // Bulk operations
    void clear(uint8_t paletteIndex = 0);
    void fill(uint8_t paletteIndex);

    // Load image file into the buffer at specified position
    // Automatically fits a 256-color palette using median-cut quantization
    // Returns true on success, false on failure
    bool loadFromFile(const std::string& imagePath, int destX = 0, int destY = 0, bool fitPalette = true);

    // Load a palette resource into the buffer
    void loadPalette(const PalettePtr& palette);

    // Draw text using a pixel font
    // For indexed buffers, renders text using the specified palette index
    // Only draws pixels where the glyph alpha is above a threshold (default 127)
    void drawText(const PixelFontPtr& font, const std::string& text, int x, int y, uint8_t colorIndex, uint8_t alphaThreshold = 127);

    // Palette manipulation
    void setPaletteEntry(uint8_t index, const Color& color);
    const Color& getPaletteEntry(uint8_t index) const;

    // Set entire 256-color palette at once
    void setPalette(const std::array<Color, 256>& palette);
    void setPalette(const Color* paletteData);  // Must point to 256 colors
    const std::array<Color, 256>& getPalette() const { return palette_; }

    // Upload pixel data to GPU texture (call after modifying pixels or palette)
    // This converts indexed colors to RGBA using the current palette
    void upload(IRenderer& renderer);

    // Dimensions (in logical pixels)
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

    // Position in world/layer coordinates
    void setPosition(const Vec2& pos) { position_ = pos; }
    const Vec2& getPosition() const { return position_; }

    // Scale for "chunky pixel" effect
    void setScale(float scale) { scale_ = scale; }
    float getScale() const { return scale_; }

    // Visibility
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const override { return visible_; }

    // ILayerAttachable interface
    void render(IRenderer& renderer, const Vec2& layerOffset, float opacity) override;

    // Get underlying texture (for rendering)
    TexturePtr getTexture() const { return texture_; }

    // Check if needs re-upload
    bool isDirty() const { return dirty_; }
    void markDirty() { dirty_ = true; }
    void markClean() { dirty_ = false; }

    // OpenGL-specific accessors (for GLRenderer)
    unsigned int getGLIndexTexture() const { return glIndexTexture_; }
    unsigned int getGLPaletteTexture() const { return glPaletteTexture_; }
    void setGLIndexTexture(unsigned int tex) { glIndexTexture_ = tex; }
    void setGLPaletteTexture(unsigned int tex) { glPaletteTexture_ = tex; }

    // Separate dirty tracking for GL shader path
    bool arePixelsDirty() const { return pixelsDirty_; }
    bool isPaletteDirty() const { return paletteDirty_; }
    void markPixelsDirty() { pixelsDirty_ = true; dirty_ = true; }
    void markPaletteDirty() { paletteDirty_ = true; dirty_ = true; }
    void markPixelsClean() { pixelsDirty_ = false; }
    void markPaletteClean() { paletteDirty_ = false; }

    // Direct access to pixel and palette data (for GL upload)
    const uint8_t* getPixelData() const { return pixels_.data(); }
    const Color* getPaletteData() const { return palette_.data(); }

private:
    int width_;
    int height_;
    std::vector<uint8_t> pixels_;  // width * height palette indices
    std::array<Color, 256> palette_;  // 256-color palette
    TexturePtr texture_;  // For SDL renderer (CPU-based conversion)
    Vec2 position_{0.0f, 0.0f};
    float scale_ = 1.0f;
    bool visible_ = true;
    bool dirty_ = true;

    // OpenGL shader path
    unsigned int glIndexTexture_ = 0;   // R8 texture with indices
    unsigned int glPaletteTexture_ = 0; // 256x1 RGBA texture with palette
    bool pixelsDirty_ = true;           // Pixels need upload to GL
    bool paletteDirty_ = true;          // Palette needs upload to GL
};

using IndexedPixelBufferPtr = std::shared_ptr<IndexedPixelBuffer>;

} // namespace Engine
