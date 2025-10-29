#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/IUpdateable.h"
#include "engine/IndexedPixelBuffer.h"
#include "engine/GLRenderer.h"
#include "engine/Palette.h"
#include "engine/PixelFont.h"
#include "engine/ResourceManager.h"
#include "engine/Layer.h"
#include "engine/FPSCounter.h"
#include "engine/Logger.h"
#include <SDL.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>

// 3D Vector math
struct Vec3 {
    float x, y, z;

    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }

    // Cross product for normal calculation
    Vec3 cross(const Vec3& other) const {
        return Vec3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    // Dot product for back-face culling
    float dot(const Vec3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    void normalize() {
        float len = std::sqrt(x*x + y*y + z*z);
        if (len > 0.0001f) {
            x /= len;
            y /= len;
            z /= len;
        }
    }
};

// Polygon face (indices into vertex array)
struct Polygon {
    std::vector<int> vertices;  // Vertex indices in CCW order
    uint8_t color;

    Polygon(std::initializer_list<int> verts, uint8_t col)
        : vertices(verts), color(col) {}
};

// 3D mesh with polygons
struct Mesh3D {
    std::vector<Vec3> vertices;
    std::vector<Polygon> polygons;

    // Create a cube mesh
    static Mesh3D createCube(float size = 30.0f) {
        Mesh3D mesh;
        float h = size / 2.0f;

        // 8 vertices
        mesh.vertices = {
            Vec3(-h, -h, -h), Vec3(h, -h, -h), Vec3(h, h, -h), Vec3(-h, h, -h),  // Front
            Vec3(-h, -h, h),  Vec3(h, -h, h),  Vec3(h, h, h),  Vec3(-h, h, h)     // Back
        };

        // 6 faces (CCW winding when viewed from outside)
        mesh.polygons = {
            Polygon({0, 1, 2, 3}, 2),  // Front (red)
            Polygon({5, 4, 7, 6}, 3),  // Back (green)
            Polygon({4, 0, 3, 7}, 4),  // Left (cyan)
            Polygon({1, 5, 6, 2}, 5),  // Right (yellow)
            Polygon({3, 2, 6, 7}, 6),  // Top (magenta)
            Polygon({4, 5, 1, 0}, 2)   // Bottom (red)
        };

        return mesh;
    }

    // Create a pyramid mesh
    static Mesh3D createPyramid(float size = 30.0f) {
        Mesh3D mesh;
        float h = size / 2.0f;
        float apex = size * 1.3f;

        // 5 vertices (4 base + 1 apex)
        mesh.vertices = {
            Vec3(-h, h, -h),   // Base 0
            Vec3(h, h, -h),    // Base 1
            Vec3(h, h, h),     // Base 2
            Vec3(-h, h, h),    // Base 3
            Vec3(0, -apex, 0)  // Apex 4
        };

        // 5 faces (4 triangular sides + 1 square base)
        mesh.polygons = {
            Polygon({0, 1, 4}, 3),     // Side 1 (green)
            Polygon({1, 2, 4}, 4),     // Side 2 (cyan)
            Polygon({2, 3, 4}, 5),     // Side 3 (yellow)
            Polygon({3, 0, 4}, 6),     // Side 4 (magenta)
            Polygon({3, 2, 1, 0}, 2)   // Base (red)
        };

        return mesh;
    }
};

// Copper bar structure
struct CopperBar {
    bool isVertical;      // true = vertical bar, false = horizontal bar
    float pos;            // Position (Y for horizontal, X for vertical)
    float velocity;       // Movement speed
    float depth;          // Z-depth (for sorting)
    int paletteStart;     // Palette start index
    float sinePhase;      // Phase offset for sine wave
    float sineSpeed;      // How fast the sine wave oscillates
    float sineAmplitude;  // How much perpendicular movement
    int size;             // Size (height for horizontal, width for vertical)
};

// Background scroller on layer 0
class BackgroundScroller : public Engine::GameObject,
                           public Engine::IUpdateable {
public:
    BackgroundScroller() : GameObject("BackgroundScroller") {
        // Initialize horizontal copper bars
        bars_.push_back({false, 20.0f, 50.0f, 0.3f, 16, 0.0f, 1.2f, 30.0f, 32});     // Horizontal Blue, near
        bars_.push_back({false, 70.0f, -40.0f, 0.8f, 48, 1.5f, 0.8f, 40.0f, 28});    // Horizontal Red, far
        bars_.push_back({false, 120.0f, 60.0f, 0.5f, 16, 3.0f, 1.5f, 35.0f, 30});    // Horizontal Blue, mid
        bars_.push_back({false, 170.0f, -56.0f, 0.7f, 48, 4.5f, 1.0f, 25.0f, 26});   // Horizontal Red, mid-far

        // Initialize vertical copper bars (80=green, 112=yellow)
        bars_.push_back({true, 40.0f, 45.0f, 0.4f, 80, 0.5f, 1.0f, 25.0f, 28});      // Vertical Green, near
        bars_.push_back({true, 120.0f, -50.0f, 0.6f, 112, 2.0f, 1.3f, 30.0f, 24});   // Vertical Yellow, mid
        bars_.push_back({true, 200.0f, 55.0f, 0.35f, 80, 3.5f, 0.9f, 35.0f, 26});    // Vertical Green, far
        bars_.push_back({true, 280.0f, -48.0f, 0.55f, 112, 5.0f, 1.1f, 28.0f, 30});  // Vertical Yellow, mid
    }

    void onAttached() override {
        LOG_INFO("BackgroundScroller attached!");

        auto& resourceManager = getEngine()->getResourceManager();

        font_ = resourceManager.loadPixelFont(
            "atascii",
            "../examples/resources/atascii.gif",
            16, 16, 16, "", 0
        );

        if (!font_) {
            LOG_ERROR("Failed to load font!");
            return;
        }

        screen_ = std::make_shared<Engine::IndexedPixelBuffer>(320, 200);
        screen_->setPosition(Engine::Vec2{0, 0});
        screen_->setScale(3.0f);

        // Create palette with copper bar gradients
        std::array<Engine::Color, 256> palette;
        palette[0] = Engine::Color{0, 0, 0, 255};        // Black background
        palette[1] = Engine::Color{170, 255, 238, 255};  // Cyan text

        // Copper bar gradient palettes
        // Blue gradient (indices 16-47 = 32 colors): dark blue -> bright blue -> dark blue
        for (int i = 0; i < 32; ++i) {
            float t = i / 31.0f;
            float brightness = std::sin(t * M_PI);

            uint8_t r = 0;
            uint8_t g = 0;
            uint8_t b = static_cast<uint8_t>(brightness * 255);

            palette[16 + i] = Engine::Color{r, g, b, 255};
        }

        // Red gradient (indices 48-79 = 32 colors): dark red -> bright red -> dark red
        for (int i = 0; i < 32; ++i) {
            float t = i / 31.0f;
            float brightness = std::sin(t * M_PI);

            uint8_t r = static_cast<uint8_t>(brightness * 255);
            uint8_t g = 0;
            uint8_t b = 0;

            palette[48 + i] = Engine::Color{r, g, b, 255};
        }

        // Green gradient (indices 80-111 = 32 colors): dark green -> bright green -> dark green
        for (int i = 0; i < 32; ++i) {
            float t = i / 31.0f;
            float brightness = std::sin(t * M_PI);

            uint8_t r = 0;
            uint8_t g = static_cast<uint8_t>(brightness * 255);
            uint8_t b = 0;

            palette[80 + i] = Engine::Color{r, g, b, 255};
        }

        // Yellow gradient (indices 112-143 = 32 colors): dark yellow -> bright yellow -> dark yellow
        for (int i = 0; i < 32; ++i) {
            float t = i / 31.0f;
            float brightness = std::sin(t * M_PI);

            uint8_t r = static_cast<uint8_t>(brightness * 255);
            uint8_t g = static_cast<uint8_t>(brightness * 255);
            uint8_t b = 0;

            palette[112 + i] = Engine::Color{r, g, b, 255};
        }

        // Fill remaining with grayscale
        for (int i = 144; i < 256; ++i) {
            uint8_t gray = static_cast<uint8_t>(i);
            palette[i] = Engine::Color{gray, gray, gray, 255};
        }

        screen_->setPalette(palette);

        auto& layers = getEngine()->getLayers();
        if (layers.size() > 0) {
            layers[0]->addIndexedPixelBuffer(screen_);
        }

        LOG_INFO("Background scroller initialized on layer 0!");
    }

    void update(float deltaTime) override {
        time_ += deltaTime;
        scrollX_ += scrollSpeed_ * deltaTime;

        screen_->clear(0);  // Black background

        // Draw copper bars
        drawCopperBars();

        const std::string message = "    COPPER BARS    "
                                   "HIDDEN LINE REMOVAL    "
                                   "POLYGON 3D OBJECTS    ";

        int charWidth = font_->getCharWidth();
        int textY = 180;

        for (size_t i = 0; i < message.length(); ++i) {
            int charX = static_cast<int>(scrollX_) + (i * charWidth);
            int messageWidth = message.length() * charWidth;

            while (charX >= 320) charX -= messageWidth;
            while (charX < -charWidth) charX += messageWidth;

            if (charX > -charWidth && charX < 320) {
                std::string singleChar(1, message[i]);
                screen_->drawText(font_, singleChar, charX, textY, 1);
            }
        }
    }

    void drawCopperBars() {
        // Update bar positions
        for (auto& bar : bars_) {
            bar.pos += bar.velocity * 0.016f;  // Assume ~60fps

            // Bounce at edges
            if (bar.isVertical) {
                // Vertical bars move left/right
                if (bar.pos < 0) {
                    bar.pos = 0;
                    bar.velocity = -bar.velocity;
                } else if (bar.pos > 320 - bar.size) {
                    bar.pos = 320 - bar.size;
                    bar.velocity = -bar.velocity;
                }
            } else {
                // Horizontal bars move up/down
                if (bar.pos < 0) {
                    bar.pos = 0;
                    bar.velocity = -bar.velocity;
                } else if (bar.pos > 200 - bar.size) {
                    bar.pos = 200 - bar.size;
                    bar.velocity = -bar.velocity;
                }
            }
        }

        // Sort bars by depth (far to near, so far bars draw first)
        std::vector<CopperBar*> sortedBars;
        for (auto& bar : bars_) {
            sortedBars.push_back(&bar);
        }
        std::sort(sortedBars.begin(), sortedBars.end(),
                  [](CopperBar* a, CopperBar* b) { return a->depth > b->depth; });

        // Draw bars in depth order
        for (auto* bar : sortedBars) {
            // Calculate sine wave offset
            float sineOffset = std::sin(time_ * bar->sineSpeed + bar->sinePhase) * bar->sineAmplitude;

            if (bar->isVertical) {
                // Draw vertical gradient bar with sine wave
                for (int x = 0; x < bar->size; ++x) {
                    int screenX = static_cast<int>(bar->pos) + x;
                    if (screenX < 0 || screenX >= 320) continue;

                    // Calculate gradient position (0 to 31)
                    int gradientPos = (x * 32) / bar->size;
                    uint8_t colorIndex = bar->paletteStart + gradientPos;

                    // Draw vertical line with sine wave distortion
                    for (int y = 0; y < 200; ++y) {
                        // Apply sine wave based on X position within the bar
                        float xProgress = static_cast<float>(x) / bar->size;
                        int yOffset = static_cast<int>(sineOffset * xProgress);
                        int screenY = y + yOffset;

                        // Wrap around vertically
                        if (screenY < 0) screenY += 200;
                        if (screenY >= 200) screenY -= 200;

                        screen_->setPixel(screenX, screenY, colorIndex);
                    }
                }
            } else {
                // Draw horizontal gradient bar with sine wave
                for (int y = 0; y < bar->size; ++y) {
                    int screenY = static_cast<int>(bar->pos) + y;
                    if (screenY < 0 || screenY >= 200) continue;

                    // Calculate gradient position (0 to 31)
                    int gradientPos = (y * 32) / bar->size;
                    uint8_t colorIndex = bar->paletteStart + gradientPos;

                    // Draw horizontal line with sine wave distortion
                    for (int x = 0; x < 320; ++x) {
                        // Apply sine wave based on Y position within the bar
                        float yProgress = static_cast<float>(y) / bar->size;
                        int xOffset = static_cast<int>(sineOffset * yProgress);
                        int screenX = x + xOffset;

                        // Wrap around horizontally
                        if (screenX < 0) screenX += 320;
                        if (screenX >= 320) screenX -= 320;

                        screen_->setPixel(screenX, screenY, colorIndex);
                    }
                }
            }
        }
    }

    void onDestroy() override {
        if (screen_) {
            auto& layers = getEngine()->getLayers();
            if (!layers.empty()) {
                layers[0]->removeIndexedPixelBuffer(screen_);
            }
        }
    }

private:
    std::shared_ptr<Engine::IndexedPixelBuffer> screen_;
    Engine::PixelFontPtr font_;
    float time_ = 0.0f;
    float scrollX_ = 320.0f;
    float scrollSpeed_ = -40.0f;
    std::vector<CopperBar> bars_;
};

// Vector objects with hidden line removal on layer 1
class VectorObjects : public Engine::GameObject,
                      public Engine::IUpdateable {
public:
    VectorObjects() : GameObject("VectorObjects") {
        cube_ = Mesh3D::createCube(30.0f);
        pyramid_ = Mesh3D::createPyramid(30.0f);
    }

    void onAttached() override {
        LOG_INFO("VectorObjects attached!");

        vectorLayer_ = std::make_shared<Engine::IndexedPixelBuffer>(320, 200);
        vectorLayer_->setPosition(Engine::Vec2{0, 0});
        vectorLayer_->setScale(3.0f);

        // Create palette with transparent background and colorful lines
        std::array<Engine::Color, 256> palette;
        palette[0] = Engine::Color{0, 0, 0, 0};        // Transparent
        palette[1] = Engine::Color{255, 255, 255, 255}; // White
        palette[2] = Engine::Color{255, 0, 0, 255};     // Red
        palette[3] = Engine::Color{0, 255, 0, 255};     // Green
        palette[4] = Engine::Color{0, 255, 255, 255};   // Cyan
        palette[5] = Engine::Color{255, 255, 0, 255};   // Yellow
        palette[6] = Engine::Color{255, 0, 255, 255};   // Magenta

        for (int i = 7; i < 256; ++i) {
            uint8_t gray = static_cast<uint8_t>(i);
            palette[i] = Engine::Color{gray, gray, gray, 255};
        }

        vectorLayer_->setPalette(palette);

        auto& layers = getEngine()->getLayers();
        if (layers.size() > 1) {
            layers[1]->addIndexedPixelBuffer(vectorLayer_);
        }

        LOG_INFO("Vector objects initialized on layer 1!");
    }

    void update(float deltaTime) override {
        time_ += deltaTime;

        vectorLayer_->clear(0);  // Clear to transparent

        // Draw both objects with different rotations
        drawMesh(cube_, 80, 80, time_ * 0.5f, time_ * 0.5f);
        drawMesh(pyramid_, 240, 80, time_ * 0.6f, time_ * 0.4f);
    }

    void drawMesh(const Mesh3D& mesh, int centerX, int centerY, float rotY, float rotX) {
        // Transform vertices
        std::vector<Vec3> transformed(mesh.vertices.size());

        float cosY = std::cos(rotY);
        float sinY = std::sin(rotY);
        float cosX = std::cos(rotX);
        float sinX = std::sin(rotX);

        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            Vec3 v = mesh.vertices[i];

            // Rotate around Y axis
            float xr = v.x * cosY - v.z * sinY;
            float zr = v.x * sinY + v.z * cosY;

            // Rotate around X axis
            float yr = v.y * cosX - zr * sinX;
            float zr2 = v.y * sinX + zr * cosX;

            transformed[i] = Vec3(xr, yr, zr2);
        }

        // View direction (camera looking down -Z axis)
        Vec3 viewDir(0, 0, -1);

        // Draw each polygon with back-face culling
        for (const auto& poly : mesh.polygons) {
            if (poly.vertices.size() < 3) continue;

            // Get first 3 vertices to calculate normal
            Vec3 v0 = transformed[poly.vertices[0]];
            Vec3 v1 = transformed[poly.vertices[1]];
            Vec3 v2 = transformed[poly.vertices[2]];

            // Calculate face normal using cross product
            Vec3 edge1 = v1 - v0;
            Vec3 edge2 = v2 - v0;
            Vec3 normal = edge1.cross(edge2);
            normal.normalize();

            // Back-face culling: only draw if normal faces camera (dot product > 0)
            float dot = normal.dot(viewDir);
            if (dot <= 0) {
                continue;  // Back face, skip it
            }

            // Draw polygon edges
            for (size_t i = 0; i < poly.vertices.size(); ++i) {
                int v0Idx = poly.vertices[i];
                int v1Idx = poly.vertices[(i + 1) % poly.vertices.size()];

                Vec3 p0 = transformed[v0Idx];
                Vec3 p1 = transformed[v1Idx];

                // Perspective projection
                float focalLength = 200.0f;  // Camera distance
                float cameraZ = 150.0f;      // Push objects away from camera

                // Apply perspective division
                float z0 = p0.z + cameraZ;
                float z1 = p1.z + cameraZ;

                // Avoid division by zero
                if (z0 < 1.0f) z0 = 1.0f;
                if (z1 < 1.0f) z1 = 1.0f;

                // Project to 2D with perspective
                int x0 = centerX + static_cast<int>((p0.x * focalLength) / z0);
                int y0 = centerY + static_cast<int>((p0.y * focalLength) / z0);
                int x1 = centerX + static_cast<int>((p1.x * focalLength) / z1);
                int y1 = centerY + static_cast<int>((p1.y * focalLength) / z1);

                vectorLayer_->drawLine(x0, y0, x1, y1, poly.color);
            }
        }
    }

    void onDestroy() override {
        if (vectorLayer_) {
            auto& layers = getEngine()->getLayers();
            if (layers.size() > 1) {
                layers[1]->removeIndexedPixelBuffer(vectorLayer_);
            }
        }
    }

private:
    std::shared_ptr<Engine::IndexedPixelBuffer> vectorLayer_;
    Mesh3D cube_;
    Mesh3D pyramid_;
    float time_ = 0.0f;
};

// FPS counter
class FPSDisplay : public Engine::GameObject,
                   public Engine::IUpdateable {
public:
    FPSDisplay() : GameObject("FPSDisplay"), fpsCounter_(60) {}

    void update(float deltaTime) override {
        fpsCounter_.update();

        timeSinceUpdate_ += deltaTime;
        if (timeSinceUpdate_ >= 1.0f) {
            std::cout << "FPS: " << fpsCounter_.getFPS()
                      << " | Vector Objects Demo" << std::endl;
            timeSinceUpdate_ = 0.0f;
        }
    }

private:
    Engine::FPSCounter fpsCounter_;
    float timeSinceUpdate_ = 0.0f;
};

int main(int argc, char* argv[]) {
    Engine::EngineConfig config;
    config.title = "3D Vector Objects - Hidden Line Removal";
    config.width = 960;
    config.height = 600;
    config.logLevel = Engine::LogLevel::Info;
    config.logToFile = false;
    config.showTimestamp = true;
    config.showWallClock = true;
    config.showFrame = true;
    config.showLogLevel = true;

    Engine::Engine engine;

    auto glRenderer = std::make_unique<Engine::GLRenderer>();
    if (!glRenderer->init(config.title, config.width, config.height)) {
        LOG_ERROR("Failed to initialize GLRenderer!");
        return 1;
    }

    if (!engine.init(config, std::move(glRenderer))) {
        LOG_ERROR("Failed to initialize engine!");
        return 1;
    }

    // Create two layers - 0 for background, 1 for vectors
    auto bgLayer = engine.createLayer(0);
    auto vectorLayer = engine.createLayer(1);

    // Create GameObjects
    auto background = std::make_shared<BackgroundScroller>();
    auto vectors = std::make_shared<VectorObjects>();
    auto fpsDisplay = std::make_shared<FPSDisplay>();

    engine.attachGameObject(background);
    engine.attachGameObject(vectors);
    engine.attachGameObject(fpsDisplay);

    std::cout << "\n=== 3D Vector Objects + Copper Bars Demo ===" << std::endl;
    std::cout << "Classic demoscene effects!" << std::endl;
    std::cout << "  - Animated copper bars" << std::endl;
    std::cout << "  - Gradient palette cycling" << std::endl;
    std::cout << "  - 3D polygon objects" << std::endl;
    std::cout << "  - Back-face culling" << std::endl;
    std::cout << "\nControls:" << std::endl;
    std::cout << "  ESC - Quit\n" << std::endl;

    while (engine.isRunning()) {
        if (!engine.pollEvents()) {
            break;
        }

        if (engine.getInput().isKeyPressed(SDLK_ESCAPE)) {
            break;
        }

        engine.update();
        engine.render();
    }

    engine.shutdown();

    std::cout << "Vector objects demo shut down successfully!" << std::endl;
    return 0;
}
