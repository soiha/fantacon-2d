#include "engine/PixelBuffer.h"
#include "engine/IRenderer.h"
#include <SDL_image.h>
#include <algorithm>
#include <iostream>

namespace Engine {

PixelBuffer::PixelBuffer(int width, int height)
    : width_(width)
    , height_(height)
    , pixels_(width * height, Color{0, 0, 0, 0}) {
    // Texture will be created on first upload
}

void PixelBuffer::setPixel(int x, int y, const Color& color) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return;  // Out of bounds
    }
    pixels_[y * width_ + x] = color;
    dirty_ = true;
}

Color PixelBuffer::getPixel(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return Color{0, 0, 0, 0};  // Out of bounds returns transparent
    }
    return pixels_[y * width_ + x];
}

void PixelBuffer::clear(const Color& color) {
    std::fill(pixels_.begin(), pixels_.end(), color);
    dirty_ = true;
}

void PixelBuffer::fill(const Color& color) {
    clear(color);
}

bool PixelBuffer::loadFromFile(const std::string& imagePath, int destX, int destY) {
    // Load image using SDL_image
    SDL_Surface* surface = IMG_Load(imagePath.c_str());
    if (!surface) {
        std::cerr << "Failed to load image: " << imagePath << " - " << IMG_GetError() << std::endl;
        return false;
    }

    // Convert surface to RGBA8888 format for consistent pixel access
    SDL_Surface* convertedSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);

    if (!convertedSurface) {
        std::cerr << "Failed to convert surface format" << std::endl;
        return false;
    }

    // Copy pixels from surface to buffer
    SDL_LockSurface(convertedSurface);
    Uint32* pixels = static_cast<Uint32*>(convertedSurface->pixels);

    for (int y = 0; y < convertedSurface->h; ++y) {
        for (int x = 0; x < convertedSurface->w; ++x) {
            int bufX = destX + x;
            int bufY = destY + y;

            // Skip pixels outside buffer bounds
            if (bufX < 0 || bufX >= width_ || bufY < 0 || bufY >= height_) {
                continue;
            }

            // Extract RGBA components (RGBA32 format: 0xRRGGBBAA)
            Uint32 pixel = pixels[y * convertedSurface->w + x];
            Color color;
            color.r = (pixel >> 24) & 0xFF;
            color.g = (pixel >> 16) & 0xFF;
            color.b = (pixel >> 8) & 0xFF;
            color.a = pixel & 0xFF;

            pixels_[bufY * width_ + bufX] = color;
        }
    }

    SDL_UnlockSurface(convertedSurface);
    SDL_FreeSurface(convertedSurface);

    dirty_ = true;
    return true;
}

void PixelBuffer::drawText(const PixelFontPtr& font, const std::string& text, int x, int y) {
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

        // Blit glyph to buffer with alpha blending
        for (int gy = 0; gy < charHeight; ++gy) {
            for (int gx = 0; gx < charWidth; ++gx) {
                int bufX = cursorX + gx;
                int bufY = cursorY + gy;

                // Skip if out of bounds
                if (bufX < 0 || bufX >= width_ || bufY < 0 || bufY >= height_) {
                    continue;
                }

                const Color& glyphPixel = glyph[gy * charWidth + gx];

                // Alpha blending: blend glyph onto existing pixel
                if (glyphPixel.a > 0) {
                    int bufIdx = bufY * width_ + bufX;
                    Color& destPixel = pixels_[bufIdx];

                    if (glyphPixel.a == 255) {
                        // Fully opaque, just copy
                        destPixel = glyphPixel;
                    } else {
                        // Alpha blend
                        float alpha = glyphPixel.a / 255.0f;
                        float invAlpha = 1.0f - alpha;

                        destPixel.r = static_cast<uint8_t>(glyphPixel.r * alpha + destPixel.r * invAlpha);
                        destPixel.g = static_cast<uint8_t>(glyphPixel.g * alpha + destPixel.g * invAlpha);
                        destPixel.b = static_cast<uint8_t>(glyphPixel.b * alpha + destPixel.b * invAlpha);
                        destPixel.a = std::min(255, static_cast<int>(glyphPixel.a + destPixel.a));
                    }
                }
            }
        }

        // Advance cursor
        cursorX += charWidth;
    }

    markDirty();
}

void PixelBuffer::upload(IRenderer& renderer) {
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

    // Update texture with pixel data
    renderer.updateTexture(*texture_, pixels_.data(), width_, height_);
    dirty_ = false;
}

void PixelBuffer::render(IRenderer& renderer, const Vec2& layerOffset, float opacity) {
    if (!visible_) {
        return;
    }

    // Upload pixels if dirty
    upload(renderer);

    renderer.renderPixelBuffer(*this, layerOffset, opacity);
}

} // namespace Engine
