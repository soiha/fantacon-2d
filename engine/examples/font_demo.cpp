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

// GameObject that demonstrates C64-style text rendering
class C64TextDemo : public Engine::GameObject,
                    public Engine::IUpdateable {
public:
    C64TextDemo() : GameObject("C64TextDemo") {}

    void onAttached() override {
        LOG_INFO("C64TextDemo attached!");

        // Get resource manager from engine
        auto& resourceManager = getEngine()->getResourceManager();

        // Load ATASCII font - 256x256 with perfect 16x16 cells
        // Extract full 16x16 to see the actual layout
        c64Font_ = resourceManager.loadPixelFont(
            "atascii",
            "examples/resources/atascii.gif",
            16,  // char width (use full 16x16 cell)
            16,  // char height
            16,  // chars per row
            "",  // use standard ASCII mapping
            0
        );

        if (!c64Font_) {
            LOG_ERROR("Failed to load C64 font!");
            return;
        }

        // Create indexed pixel buffer (320x200 - classic retro resolution)
        screen_ = std::make_shared<Engine::IndexedPixelBuffer>(320, 200);
        // Use (0,0) position for perfect pixel alignment
        screen_->setPosition(Engine::Vec2{0, 0});
        screen_->setScale(3.0f);  // Use integer scale for perfect pixel alignment

        // Create C64-inspired palette
        std::array<Engine::Color, 256> c64Palette;

        // C64 classic colors (first 16 entries)
        c64Palette[0]  = Engine::Color{0, 0, 0, 255};         // Black
        c64Palette[1]  = Engine::Color{255, 255, 255, 255};   // White
        c64Palette[2]  = Engine::Color{136, 0, 0, 255};       // Red
        c64Palette[3]  = Engine::Color{170, 255, 238, 255};   // Cyan
        c64Palette[4]  = Engine::Color{204, 68, 204, 255};    // Purple
        c64Palette[5]  = Engine::Color{0, 204, 85, 255};      // Green
        c64Palette[6]  = Engine::Color{0, 0, 170, 255};       // Blue
        c64Palette[7]  = Engine::Color{238, 238, 119, 255};   // Yellow
        c64Palette[8]  = Engine::Color{221, 136, 85, 255};    // Orange
        c64Palette[9]  = Engine::Color{102, 68, 0, 255};      // Brown
        c64Palette[10] = Engine::Color{255, 119, 119, 255};   // Light Red
        c64Palette[11] = Engine::Color{51, 51, 51, 255};      // Dark Grey
        c64Palette[12] = Engine::Color{119, 119, 119, 255};   // Grey
        c64Palette[13] = Engine::Color{170, 255, 102, 255};   // Light Green
        c64Palette[14] = Engine::Color{0, 136, 255, 255};     // Light Blue
        c64Palette[15] = Engine::Color{187, 187, 187, 255};   // Light Grey

        // Fill remaining with grayscale
        for (int i = 16; i < 256; ++i) {
            uint8_t gray = static_cast<uint8_t>(i);
            c64Palette[i] = Engine::Color{gray, gray, gray, 255};
        }

        screen_->setPalette(c64Palette);

        // Clear to C64 blue
        screen_->clear(6);

        // Draw some text with the C64 font
        drawScreen();

        // Add to first layer
        auto& layers = getEngine()->getLayers();
        if (!layers.empty()) {
            layers[0]->addIndexedPixelBuffer(screen_);
        }

        LOG_INFO("Created 320x200 indexed screen with C64 font!");
    }

    void drawScreen() {
        // Draw a simple test pattern first - a rectangle
        // This helps debug if the issue is rendering or text drawing
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                screen_->setPixel(x, y, 14);  // Light blue
            }
        }

        // Start text below the rectangle to avoid overlap
        int y = 70;

        // Draw some classic C64 text!
        screen_->drawText(c64Font_, "HELLO WORLD", 0, y, 14);
        y += 16;
        screen_->drawText(c64Font_, "0123456789", 0, y, 14);
        y += 16;
        screen_->drawText(c64Font_, "COMMODORE 64", 0, y, 14);
        y += 16;
        screen_->drawText(c64Font_, "@!#$%&*+-=<>?", 0, y, 14);
    }

    void update(float deltaTime) override {
        // No animation in debug mode
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
    Engine::PixelFontPtr c64Font_;
    float time_ = 0.0f;
};

// Simple FPS counter
class FPSDisplay : public Engine::GameObject,
                   public Engine::IUpdateable {
public:
    FPSDisplay() : GameObject("FPSDisplay"), fpsCounter_(60) {}

    void update(float deltaTime) override {
        fpsCounter_.update();

        timeSinceUpdate_ += deltaTime;
        if (timeSinceUpdate_ >= 1.0f) {
            std::cout << "FPS: " << fpsCounter_.getFPS()
                      << " | C64 Font Demo" << std::endl;
            timeSinceUpdate_ = 0.0f;
        }
    }

private:
    Engine::FPSCounter fpsCounter_;
    float timeSinceUpdate_ = 0.0f;
};

int main(int argc, char* argv[]) {
    // Configure engine
    Engine::EngineConfig config;
    config.title = "C64 Pixel Font Demo";
    config.width = 960;  // 320 * 3 = 960 for pixel-perfect rendering
    config.height = 600; // 200 * 3 = 600 for pixel-perfect rendering
    config.logLevel = Engine::LogLevel::Info;
    config.logToFile = false;
    config.showTimestamp = true;
    config.showWallClock = true;
    config.showFrame = true;
    config.showLogLevel = true;

    // Create engine with GLRenderer
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

    // Create a layer
    auto layer = engine.createLayer(0);

    // Create and attach GameObjects
    auto textDemo = std::make_shared<C64TextDemo>();
    auto fpsDisplay = std::make_shared<FPSDisplay>();

    engine.attachGameObject(textDemo);
    engine.attachGameObject(fpsDisplay);

    // Info text
    std::cout << "\n=== C64 Pixel Font Demo ===" << std::endl;
    std::cout << "Classic Commodore 64 style text rendering!" << std::endl;
    std::cout << "  - 320x200 indexed pixel buffer" << std::endl;
    std::cout << "  - Authentic C64 charset" << std::endl;
    std::cout << "  - 16-color C64 palette" << std::endl;
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

    std::cout << "C64 font demo shut down successfully!" << std::endl;
    return 0;
}
