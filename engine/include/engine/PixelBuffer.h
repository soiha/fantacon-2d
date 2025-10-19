#pragma once

#include "Types.h"
#include "Texture.h"
#include "PixelFont.h"
#include <vector>
#include <memory>
#include <string>

namespace Engine {

class IRenderer;

// A bitmap buffer for direct pixel manipulation
// Perfect for debug drawing, effects, minimaps, or retro "chunky pixel" graphics
// Pixels are manipulated in RAM and uploaded to GPU as needed
class PixelBuffer {
public:
    PixelBuffer(int width, int height);
    ~PixelBuffer() = default;

    // Pixel manipulation (coordinates are clamped to buffer size)
    void setPixel(int x, int y, const Color& color);
    Color getPixel(int x, int y) const;

    // Bulk operations
    void clear(const Color& color = Color{0, 0, 0, 0}); // Clear to transparent black by default
    void fill(const Color& color);  // Same as clear, for consistency

    // Load image file into the buffer at specified position
    // Returns true on success, false on failure
    bool loadFromFile(const std::string& imagePath, int destX = 0, int destY = 0);

    // Draw text using a pixel font
    // For RGBA buffers, renders using the font's original colors (with alpha blending)
    void drawText(const PixelFontPtr& font, const std::string& text, int x, int y);

    // Upload pixel data to GPU texture (call after modifying pixels)
    // This is automatically called during rendering if dirty
    void upload(IRenderer& renderer);

    // Dimensions (in logical pixels)
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

    // Position in world/layer coordinates
    void setPosition(const Vec2& pos) { position_ = pos; }
    const Vec2& getPosition() const { return position_; }

    // Scale for "chunky pixel" effect (1.0 = normal, 2.0 = each pixel doubled, etc.)
    void setScale(float scale) { scale_ = scale; }
    float getScale() const { return scale_; }

    // Visibility
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }

    // Get underlying texture (for rendering)
    TexturePtr getTexture() const { return texture_; }

    // Check if needs re-upload
    bool isDirty() const { return dirty_; }
    void markDirty() { dirty_ = true; }
    void markClean() { dirty_ = false; }

private:
    int width_;
    int height_;
    std::vector<Color> pixels_;  // width * height colors (row-major: y * width + x)
    TexturePtr texture_;
    Vec2 position_{0.0f, 0.0f};
    float scale_ = 1.0f;
    bool visible_ = true;
    bool dirty_ = true;  // Need to re-upload to GPU?
};

using PixelBufferPtr = std::shared_ptr<PixelBuffer>;

} // namespace Engine
