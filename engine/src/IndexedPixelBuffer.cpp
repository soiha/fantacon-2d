#include "engine/IndexedPixelBuffer.h"
#include "engine/IRenderer.h"
#include <SDL_image.h>
#include <algorithm>
#include <vector>
#include <iostream>
#include <cmath>

namespace Engine {

IndexedPixelBuffer::IndexedPixelBuffer(int width, int height)
    : width_(width)
    , height_(height)
    , pixels_(width * height, 0) {

    // Initialize with a default grayscale palette
    for (int i = 0; i < 256; ++i) {
        uint8_t gray = static_cast<uint8_t>(i);
        palette_[i] = Color{gray, gray, gray, 255};
    }
}

void IndexedPixelBuffer::setPixel(int x, int y, uint8_t paletteIndex) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return;  // Out of bounds
    }
    pixels_[y * width_ + x] = paletteIndex;
    markPixelsDirty();
}

uint8_t IndexedPixelBuffer::getPixel(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return 0;  // Out of bounds returns index 0
    }
    return pixels_[y * width_ + x];
}

void IndexedPixelBuffer::drawLine(int x0, int y0, int x1, int y1, uint8_t paletteIndex) {
    // Bresenham's line algorithm
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        setPixel(x0, y0, paletteIndex);

        if (x0 == x1 && y0 == y1) {
            break;
        }

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void IndexedPixelBuffer::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t paletteIndex) {
    // Sort vertices by Y coordinate (y0 <= y1 <= y2)
    if (y1 < y0) { std::swap(x0, x1); std::swap(y0, y1); }
    if (y2 < y0) { std::swap(x0, x2); std::swap(y0, y2); }
    if (y2 < y1) { std::swap(x1, x2); std::swap(y1, y2); }

    // Handle degenerate cases
    if (y0 == y2) return;  // All vertices on same horizontal line

    // Fill the triangle using scanline algorithm
    auto fillScanline = [this, paletteIndex](int y, int x_start, int x_end) {
        if (x_start > x_end) std::swap(x_start, x_end);
        for (int x = x_start; x <= x_end; ++x) {
            setPixel(x, y, paletteIndex);
        }
    };

    // Interpolate X coordinates for each scanline
    for (int y = y0; y <= y2; ++y) {
        // Calculate X coordinates on the two edges at this Y
        int x_a, x_b;

        // Long edge (from y0 to y2)
        if (y2 > y0) {
            float t = static_cast<float>(y - y0) / (y2 - y0);
            x_a = static_cast<int>(x0 + t * (x2 - x0));
        } else {
            x_a = x0;
        }

        // Short edge (either y0-y1 or y1-y2)
        if (y < y1) {
            // Top half: interpolate between y0 and y1
            if (y1 > y0) {
                float t = static_cast<float>(y - y0) / (y1 - y0);
                x_b = static_cast<int>(x0 + t * (x1 - x0));
            } else {
                x_b = x0;
            }
        } else {
            // Bottom half: interpolate between y1 and y2
            if (y2 > y1) {
                float t = static_cast<float>(y - y1) / (y2 - y1);
                x_b = static_cast<int>(x1 + t * (x2 - x1));
            } else {
                x_b = x1;
            }
        }

        fillScanline(y, x_a, x_b);
    }
}

void IndexedPixelBuffer::fillRect(int x, int y, int width, int height, uint8_t paletteIndex) {
    // Clamp rectangle to buffer bounds
    int x1 = std::max(0, x);
    int y1 = std::max(0, y);
    int x2 = std::min(width_, x + width);
    int y2 = std::min(height_, y + height);

    // Nothing to draw if rectangle is completely out of bounds
    if (x1 >= x2 || y1 >= y2) {
        return;
    }

    // Optimized scanline-based fill (better cache locality than nested setPixel calls)
    for (int row = y1; row < y2; ++row) {
        // Direct memory access for the entire row span
        int offset = row * width_ + x1;
        int count = x2 - x1;
        std::fill_n(&pixels_[offset], count, paletteIndex);
    }

    markPixelsDirty();
}

void IndexedPixelBuffer::clear(uint8_t paletteIndex) {
    std::fill(pixels_.begin(), pixels_.end(), paletteIndex);
    markPixelsDirty();
}

void IndexedPixelBuffer::fill(uint8_t paletteIndex) {
    clear(paletteIndex);
}

void IndexedPixelBuffer::setPaletteEntry(uint8_t index, const Color& color) {
    palette_[index] = color;
    markPaletteDirty();
}

const Color& IndexedPixelBuffer::getPaletteEntry(uint8_t index) const {
    return palette_[index];
}

void IndexedPixelBuffer::setPalette(const std::array<Color, 256>& palette) {
    palette_ = palette;
    markPaletteDirty();
}

void IndexedPixelBuffer::setPalette(const Color* paletteData) {
    if (!paletteData) return;

    for (int i = 0; i < 256; ++i) {
        palette_[i] = paletteData[i];
    }
    markPaletteDirty();
}

namespace {
    // Helper struct for median-cut quantization
    struct ColorBox {
        std::vector<Color> colors;

        void computeRange(int& rRange, int& gRange, int& bRange) const {
            if (colors.empty()) {
                rRange = gRange = bRange = 0;
                return;
            }

            uint8_t minR = 255, maxR = 0;
            uint8_t minG = 255, maxG = 0;
            uint8_t minB = 255, maxB = 0;

            for (const auto& c : colors) {
                minR = std::min(minR, c.r);
                maxR = std::max(maxR, c.r);
                minG = std::min(minG, c.g);
                maxG = std::max(maxG, c.g);
                minB = std::min(minB, c.b);
                maxB = std::max(maxB, c.b);
            }

            rRange = maxR - minR;
            gRange = maxG - minG;
            bRange = maxB - minB;
        }

        Color getAverageColor() const {
            if (colors.empty()) return Color{0, 0, 0, 255};

            uint32_t rSum = 0, gSum = 0, bSum = 0, aSum = 0;
            for (const auto& c : colors) {
                rSum += c.r;
                gSum += c.g;
                bSum += c.b;
                aSum += c.a;
            }

            size_t count = colors.size();
            return Color{
                static_cast<uint8_t>(rSum / count),
                static_cast<uint8_t>(gSum / count),
                static_cast<uint8_t>(bSum / count),
                static_cast<uint8_t>(aSum / count)
            };
        }
    };

    // Find nearest color in palette
    uint8_t findNearestPaletteIndex(const Color& color, const std::array<Color, 256>& palette, int paletteSize) {
        int bestIndex = 0;
        int bestDistance = INT_MAX;

        for (int i = 0; i < paletteSize; ++i) {
            int dr = static_cast<int>(color.r) - palette[i].r;
            int dg = static_cast<int>(color.g) - palette[i].g;
            int db = static_cast<int>(color.b) - palette[i].b;
            int distance = dr*dr + dg*dg + db*db;

            if (distance < bestDistance) {
                bestDistance = distance;
                bestIndex = i;
            }
        }

        return static_cast<uint8_t>(bestIndex);
    }
}

bool IndexedPixelBuffer::loadFromFile(const std::string& imagePath, int destX, int destY, bool fitPalette) {
    // Load image using SDL_image
    SDL_Surface* surface = IMG_Load(imagePath.c_str());
    if (!surface) {
        std::cerr << "Failed to load image: " << imagePath << " - " << IMG_GetError() << std::endl;
        return false;
    }

    // Convert surface to RGBA8888 format
    SDL_Surface* convertedSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);

    if (!convertedSurface) {
        std::cerr << "Failed to convert surface format" << std::endl;
        return false;
    }

    SDL_LockSurface(convertedSurface);
    Uint32* pixels = static_cast<Uint32*>(convertedSurface->pixels);

    // Collect all unique colors
    std::vector<Color> imageColors;
    for (int y = 0; y < convertedSurface->h; ++y) {
        for (int x = 0; x < convertedSurface->w; ++x) {
            Uint32 pixel = pixels[y * convertedSurface->w + x];
            Color color;
            color.r = (pixel >> 24) & 0xFF;
            color.g = (pixel >> 16) & 0xFF;
            color.b = (pixel >> 8) & 0xFF;
            color.a = pixel & 0xFF;
            imageColors.push_back(color);
        }
    }

    std::array<Color, 256> newPalette = palette_;  // Start with current palette

    if (fitPalette && imageColors.size() > 1) {
        // Median-cut quantization to generate 256-color palette
        std::vector<ColorBox> boxes;
        boxes.push_back(ColorBox{imageColors});

        // Split boxes until we have 256
        while (boxes.size() < 256) {
            // Find box with largest range
            size_t largestIdx = 0;
            int largestRange = 0;

            for (size_t i = 0; i < boxes.size(); ++i) {
                int rRange, gRange, bRange;
                boxes[i].computeRange(rRange, gRange, bRange);
                int totalRange = rRange + gRange + bRange;

                if (totalRange > largestRange) {
                    largestRange = totalRange;
                    largestIdx = i;
                }
            }

            if (largestRange == 0) break;  // All boxes are single colors

            // Split the largest box
            ColorBox& box = boxes[largestIdx];
            int rRange, gRange, bRange;
            box.computeRange(rRange, gRange, bRange);

            // Split along longest axis
            if (rRange >= gRange && rRange >= bRange) {
                std::sort(box.colors.begin(), box.colors.end(),
                         [](const Color& a, const Color& b) { return a.r < b.r; });
            } else if (gRange >= bRange) {
                std::sort(box.colors.begin(), box.colors.end(),
                         [](const Color& a, const Color& b) { return a.g < b.g; });
            } else {
                std::sort(box.colors.begin(), box.colors.end(),
                         [](const Color& a, const Color& b) { return a.b < b.b; });
            }

            // Split in half
            size_t mid = box.colors.size() / 2;
            ColorBox newBox;
            newBox.colors.assign(box.colors.begin() + mid, box.colors.end());
            box.colors.erase(box.colors.begin() + mid, box.colors.end());
            boxes.push_back(newBox);
        }

        // Generate palette from box averages
        for (size_t i = 0; i < boxes.size() && i < 256; ++i) {
            newPalette[i] = boxes[i].getAverageColor();
        }

        // Fill remaining palette entries with black
        for (size_t i = boxes.size(); i < 256; ++i) {
            newPalette[i] = Color{0, 0, 0, 255};
        }

        setPalette(newPalette);
    }

    // Map pixels to palette and copy to buffer
    for (int y = 0; y < convertedSurface->h; ++y) {
        for (int x = 0; x < convertedSurface->w; ++x) {
            int bufX = destX + x;
            int bufY = destY + y;

            if (bufX < 0 || bufX >= width_ || bufY < 0 || bufY >= height_) {
                continue;
            }

            Uint32 pixel = pixels[y * convertedSurface->w + x];
            Color color;
            color.r = (pixel >> 24) & 0xFF;
            color.g = (pixel >> 16) & 0xFF;
            color.b = (pixel >> 8) & 0xFF;
            color.a = pixel & 0xFF;

            uint8_t paletteIndex = findNearestPaletteIndex(color, newPalette, 256);
            pixels_[bufY * width_ + bufX] = paletteIndex;
        }
    }

    SDL_UnlockSurface(convertedSurface);
    SDL_FreeSurface(convertedSurface);

    markPixelsDirty();
    return true;
}

void IndexedPixelBuffer::loadPalette(const PalettePtr& palette) {
    if (palette) {
        setPalette(palette->getColors());
    }
}

void IndexedPixelBuffer::drawText(const PixelFontPtr& font, const std::string& text, int x, int y, uint8_t colorIndex, uint8_t alphaThreshold) {
    if (!font) {
        return;
    }

    int cursorX = x;
    int cursorY = y;
    int charWidth = font->getCharWidth();
    int charHeight = font->getCharHeight();

    for (char c : text) {
        // Handle newline
        if (c == '\n') {
            cursorX = x;
            cursorY += charHeight;
            continue;
        }

        // Get glyph for this character
        const auto& glyph = font->getGlyph(c);
        if (glyph.empty()) {
            // Character not in font, skip it (or use space width)
            cursorX += charWidth;
            continue;
        }

        // Blit glyph to buffer
        for (int gy = 0; gy < charHeight; ++gy) {
            for (int gx = 0; gx < charWidth; ++gx) {
                int bufX = cursorX + gx;
                int bufY = cursorY + gy;

                // Skip if out of bounds
                if (bufX < 0 || bufX >= width_ || bufY < 0 || bufY >= height_) {
                    continue;
                }

                const Color& glyphPixel = glyph[gy * charWidth + gx];

                // For bitmap fonts: check if pixel is dark (below threshold) = glyph pixel
                // This handles inverted fonts where black = glyph, white = background
                if (glyphPixel.r < alphaThreshold) {
                    setPixel(bufX, bufY, colorIndex);
                }
            }
        }

        // Advance cursor
        cursorX += charWidth;
    }

    markPixelsDirty();
}

void IndexedPixelBuffer::upload(IRenderer& renderer) {
    if (!dirty_) {
        return;  // No changes, skip upload
    }

    // Create texture if it doesn't exist yet
    if (!texture_) {
        texture_ = renderer.createStreamingTexture(width_, height_);
        if (!texture_) {
            return;  // Failed to create texture
        }
    }

    // Convert indexed pixels to RGBA using the palette
    std::vector<Color> rgbaPixels(width_ * height_);
    for (int i = 0; i < width_ * height_; ++i) {
        uint8_t index = pixels_[i];
        rgbaPixels[i] = palette_[index];
    }

    // Update texture with converted pixel data
    renderer.updateTexture(*texture_, rgbaPixels.data(), width_, height_);
    dirty_ = false;
}

void IndexedPixelBuffer::render(IRenderer& renderer, const Vec2& layerOffset, float opacity) {
    if (!visible_) {
        return;
    }
    renderer.renderIndexedPixelBuffer(*this, layerOffset, opacity);
}

} // namespace Engine
