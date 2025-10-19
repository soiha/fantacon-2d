#pragma once

#include "Types.h"
#include <array>
#include <string>
#include <memory>

namespace Engine {

// 256-color palette resource for indexed color graphics
class Palette {
public:
    Palette() = default;
    ~Palette() = default;

    // Load palette from CSV file (format: r,g,b,a per line, 256 entries)
    bool loadFromFile(const std::string& path);

    // Load palette from PNG image (each pixel = one palette color, up to 256 pixels)
    bool loadFromImage(const std::string& imagePath);

    // Get/Set palette data
    const std::array<Color, 256>& getColors() const { return colors_; }
    void setColors(const std::array<Color, 256>& colors) { colors_ = colors; }

    // Get individual color
    const Color& getColor(uint8_t index) const { return colors_[index]; }
    void setColor(uint8_t index, const Color& color) { colors_[index] = color; }

    // Generate standard palettes
    static Palette createGrayscale();
    static Palette createVGA();  // Classic VGA 256-color palette
    static Palette createFireGradient();
    static Palette createRainbow();

private:
    std::array<Color, 256> colors_;
};

using PalettePtr = std::shared_ptr<Palette>;

} // namespace Engine
