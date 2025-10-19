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

// Classic scroller with palette-based color pulsing
class ColorPulseScroller : public Engine::GameObject,
                           public Engine::IUpdateable {
public:
    ColorPulseScroller() : GameObject("ColorPulseScroller") {}

    void onAttached() override {
        LOG_INFO("ColorPulseScroller attached!");

        // Get resource manager from engine
        auto& resourceManager = getEngine()->getResourceManager();

        // Load ATASCII font
        font_ = resourceManager.loadPixelFont(
            "atascii",
            "../examples/resources/atascii.gif",
            16,  // char width
            16,  // char height
            16,  // chars per row
            "",  // use standard ASCII mapping
            0
        );

        if (!font_) {
            LOG_ERROR("Failed to load font!");
            return;
        }

        // Create indexed pixel buffer (320x200 - classic retro resolution)
        screen_ = std::make_shared<Engine::IndexedPixelBuffer>(320, 200);
        screen_->setPosition(Engine::Vec2{0, 0});
        screen_->setScale(3.0f);  // 3x scale for 960x600 window

        // Create C64-inspired palette
        std::array<Engine::Color, 256> c64Palette;
        c64Palette[0]  = Engine::Color{0, 0, 0, 255};         // Black (background)
        c64Palette[1]  = Engine::Color{255, 255, 255, 255};   // White (text - will be animated)
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

        // Add to first layer
        auto& layers = getEngine()->getLayers();
        if (!layers.empty()) {
            layers[0]->addIndexedPixelBuffer(screen_);
        }

        LOG_INFO("Color pulse scroller initialized!");
    }

    void update(float deltaTime) override {
        time_ += deltaTime;
        scrollX_ += scrollSpeed_ * deltaTime;

        // Clear screen to black and render text
        screen_->clear(0);
        renderScrollText();

        // Pulse the palette color for index 1 (our text color)
        updatePalettePulse();
    }

    void renderScrollText() {
        const std::string message = "    WELCOME TO THE COLOR PULSE SCROLLER!    "
                                   "THIS EFFECT USES PALETTE ANIMATION    "
                                   "CLASSIC DEMO SCENE TECHNIQUE    "
                                   "RETRO COMPUTING AT ITS FINEST!    ";

        int charWidth = font_->getCharWidth();
        int charHeight = font_->getCharHeight();

        // Fixed Y position (centered vertically)
        int textY = 100 - charHeight / 2;

        // Render each character
        for (size_t i = 0; i < message.length(); ++i) {
            char c = message[i];

            // Calculate X position with scrolling
            int charX = static_cast<int>(scrollX_) + (i * charWidth);

            // Wrap around when scrolling off screen
            int messageWidth = message.length() * charWidth;
            while (charX >= 320) {
                charX -= messageWidth;
            }
            while (charX < -charWidth) {
                charX += messageWidth;
            }

            // Draw character if it's on screen
            if (charX > -charWidth && charX < 320) {
                // All text uses palette index 1 (which we'll animate)
                std::string singleChar(1, c);
                screen_->drawText(font_, singleChar, charX, textY, 1);
            }
        }
    }

    void updatePalettePulse() {
        // Create a smooth color pulse using sine waves for RGB channels
        // Each channel has a different phase for rainbow effect
        float r = (std::sin(time_ * 2.0f) * 0.5f + 0.5f);
        float g = (std::sin(time_ * 2.0f + 2.0944f) * 0.5f + 0.5f);  // +120 degrees
        float b = (std::sin(time_ * 2.0f + 4.1888f) * 0.5f + 0.5f);  // +240 degrees

        // Scale to bright colors (minimum 64, maximum 255)
        uint8_t red = static_cast<uint8_t>(64 + r * 191);
        uint8_t green = static_cast<uint8_t>(64 + g * 191);
        uint8_t blue = static_cast<uint8_t>(64 + b * 191);

        Engine::Color pulseColor{red, green, blue, 255};
        screen_->setPaletteEntry(1, pulseColor);
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
    float scrollX_ = 320.0f;  // Start off-screen to the right
    float scrollSpeed_ = -50.0f;  // Pixels per second (negative = scroll left)
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
                      << " | Color Pulse Scroller" << std::endl;
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
    config.title = "Color Pulse Scroller Demo";
    config.width = 960;  // 320 * 3
    config.height = 600; // 200 * 3
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
    auto scroller = std::make_shared<ColorPulseScroller>();
    auto fpsDisplay = std::make_shared<FPSDisplay>();

    engine.attachGameObject(scroller);
    engine.attachGameObject(fpsDisplay);

    // Info text
    std::cout << "\n=== Color Pulse Scroller Demo ===" << std::endl;
    std::cout << "Classic palette animation effect!" << std::endl;
    std::cout << "  - Horizontal scrolling text" << std::endl;
    std::cout << "  - Palette-based color pulsing" << std::endl;
    std::cout << "  - Rainbow color cycle" << std::endl;
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

    std::cout << "Color pulse scroller shut down successfully!" << std::endl;
    return 0;
}
