#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace Engine {

// Use GLM's vec2 for 2D vectors
using Vec2 = glm::vec2;

// Color structure (RGBA)
struct Color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    Color() = default;
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
};

// Simple rect structure (GLM doesn't have a built-in rect)
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    Rect() = default;
    Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}
};

// Anchor points for sprite positioning
enum class AnchorPoint {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Center
};

} // namespace Engine
