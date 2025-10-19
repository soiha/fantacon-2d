#include "engine/Palette.h"
#include <SDL_image.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>

namespace Engine {

bool Palette::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open palette file: " << path << std::endl;
        return false;
    }

    int index = 0;
    std::string line;

    while (std::getline(file, line) && index < 256) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        std::string token;
        int values[4] = {0, 0, 0, 255};  // Default alpha to 255
        int valueIndex = 0;

        // Parse comma-separated values
        while (std::getline(iss, token, ',') && valueIndex < 4) {
            try {
                values[valueIndex] = std::stoi(token);
            } catch (...) {
                std::cerr << "Invalid value in palette file: " << token << std::endl;
                return false;
            }
            valueIndex++;
        }

        // Must have at least RGB
        if (valueIndex < 3) {
            std::cerr << "Invalid palette entry (need at least R,G,B): " << line << std::endl;
            return false;
        }

        colors_[index] = Color{
            static_cast<uint8_t>(std::clamp(values[0], 0, 255)),
            static_cast<uint8_t>(std::clamp(values[1], 0, 255)),
            static_cast<uint8_t>(std::clamp(values[2], 0, 255)),
            static_cast<uint8_t>(std::clamp(values[3], 0, 255))
        };

        index++;
    }

    // Fill remaining entries with black if file had fewer than 256 colors
    for (int i = index; i < 256; ++i) {
        colors_[i] = Color{0, 0, 0, 255};
    }

    return true;
}

bool Palette::loadFromImage(const std::string& imagePath) {
    // Load image using SDL_image
    SDL_Surface* surface = IMG_Load(imagePath.c_str());
    if (!surface) {
        std::cerr << "Failed to load palette image: " << imagePath << " - " << IMG_GetError() << std::endl;
        return false;
    }

    // Convert to RGBA8888 format
    SDL_Surface* convertedSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);

    if (!convertedSurface) {
        std::cerr << "Failed to convert palette image format" << std::endl;
        return false;
    }

    SDL_LockSurface(convertedSurface);
    Uint32* pixels = static_cast<Uint32*>(convertedSurface->pixels);

    // Read up to 256 pixels from the image (row by row, left to right)
    int pixelCount = std::min(256, convertedSurface->w * convertedSurface->h);

    for (int i = 0; i < pixelCount; ++i) {
        Uint32 pixel = pixels[i];
        colors_[i].r = (pixel >> 24) & 0xFF;
        colors_[i].g = (pixel >> 16) & 0xFF;
        colors_[i].b = (pixel >> 8) & 0xFF;
        colors_[i].a = pixel & 0xFF;
    }

    // Fill remaining entries with black
    for (int i = pixelCount; i < 256; ++i) {
        colors_[i] = Color{0, 0, 0, 255};
    }

    SDL_UnlockSurface(convertedSurface);
    SDL_FreeSurface(convertedSurface);

    return true;
}

Palette Palette::createGrayscale() {
    Palette palette;
    for (int i = 0; i < 256; ++i) {
        uint8_t gray = static_cast<uint8_t>(i);
        palette.colors_[i] = Color{gray, gray, gray, 255};
    }
    return palette;
}

Palette Palette::createVGA() {
    Palette palette;

    // First 16 colors: Standard VGA colors
    Color vgaColors[16] = {
        {0, 0, 0, 255},       // Black
        {0, 0, 170, 255},     // Blue
        {0, 170, 0, 255},     // Green
        {0, 170, 170, 255},   // Cyan
        {170, 0, 0, 255},     // Red
        {170, 0, 170, 255},   // Magenta
        {170, 85, 0, 255},    // Brown
        {170, 170, 170, 255}, // Light Gray
        {85, 85, 85, 255},    // Dark Gray
        {85, 85, 255, 255},   // Light Blue
        {85, 255, 85, 255},   // Light Green
        {85, 255, 255, 255},  // Light Cyan
        {255, 85, 85, 255},   // Light Red
        {255, 85, 255, 255},  // Light Magenta
        {255, 255, 85, 255},  // Yellow
        {255, 255, 255, 255}  // White
    };

    for (int i = 0; i < 16; ++i) {
        palette.colors_[i] = vgaColors[i];
    }

    // Remaining colors: 6x6x6 color cube + grayscale ramp
    int idx = 16;
    for (int r = 0; r < 6; ++r) {
        for (int g = 0; g < 6; ++g) {
            for (int b = 0; b < 6; ++b) {
                if (idx < 256) {
                    palette.colors_[idx++] = Color{
                        static_cast<uint8_t>(r * 51),
                        static_cast<uint8_t>(g * 51),
                        static_cast<uint8_t>(b * 51),
                        255
                    };
                }
            }
        }
    }

    // Fill remaining with grayscale
    for (int i = idx; i < 256; ++i) {
        uint8_t gray = static_cast<uint8_t>((i - idx) * 255 / (255 - idx));
        palette.colors_[i] = Color{gray, gray, gray, 255};
    }

    return palette;
}

Palette Palette::createFireGradient() {
    Palette palette;

    for (int i = 0; i < 256; ++i) {
        float t = i / 255.0f;

        if (i < 64) {
            // Black to dark red
            float brightness = i / 64.0f;
            palette.colors_[i] = Color{
                static_cast<uint8_t>(brightness * 64),
                0,
                0,
                255
            };
        } else if (i < 128) {
            // Dark red to orange
            float t2 = (i - 64) / 64.0f;
            palette.colors_[i] = Color{
                static_cast<uint8_t>(64 + t2 * 191),
                static_cast<uint8_t>(t2 * 128),
                0,
                255
            };
        } else if (i < 192) {
            // Orange to yellow
            float t2 = (i - 128) / 64.0f;
            palette.colors_[i] = Color{
                255,
                static_cast<uint8_t>(128 + t2 * 127),
                static_cast<uint8_t>(t2 * 128),
                255
            };
        } else {
            // Yellow to white
            float t2 = (i - 192) / 63.0f;
            palette.colors_[i] = Color{
                255,
                255,
                static_cast<uint8_t>(128 + t2 * 127),
                255
            };
        }
    }

    return palette;
}

Palette Palette::createRainbow() {
    Palette palette;

    for (int i = 0; i < 256; ++i) {
        float hue = i / 255.0f * 360.0f;  // 0-360 degrees

        // Convert HSV to RGB (S=1, V=1)
        float c = 1.0f;
        float x = c * (1.0f - std::abs(std::fmod(hue / 60.0f, 2.0f) - 1.0f));
        float m = 0.0f;

        float r, g, b;
        if (hue < 60) {
            r = c; g = x; b = 0;
        } else if (hue < 120) {
            r = x; g = c; b = 0;
        } else if (hue < 180) {
            r = 0; g = c; b = x;
        } else if (hue < 240) {
            r = 0; g = x; b = c;
        } else if (hue < 300) {
            r = x; g = 0; b = c;
        } else {
            r = c; g = 0; b = x;
        }

        palette.colors_[i] = Color{
            static_cast<uint8_t>((r + m) * 255),
            static_cast<uint8_t>((g + m) * 255),
            static_cast<uint8_t>((b + m) * 255),
            255
        };
    }

    return palette;
}

} // namespace Engine
