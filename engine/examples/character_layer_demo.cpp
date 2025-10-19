#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/IUpdateable.h"
#include "engine/IndexedPixelBuffer.h"
#include "engine/CharacterLayer.h"
#include "engine/GLRenderer.h"
#include "engine/PixelFont.h"
#include "engine/ResourceManager.h"
#include "engine/Layer.h"
#include "engine/FPSCounter.h"
#include "engine/Logger.h"
#include <SDL.h>
#include <iostream>
#include <sstream>
#include <cmath>

// Terminal-like character layer demo
class TerminalDemo : public Engine::GameObject,
                     public Engine::IUpdateable {
public:
    TerminalDemo() : GameObject("TerminalDemo") {}

    void onAttached() override {
        LOG_INFO("TerminalDemo attached!");

        auto& resourceManager = getEngine()->getResourceManager();

        // Load ATASCII font (16x16 characters)
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

        // Create indexed pixel buffer (320x200)
        screen_ = std::make_shared<Engine::IndexedPixelBuffer>(320, 200);
        screen_->setPosition(Engine::Vec2{0, 0});
        screen_->setScale(3.0f);  // 3x scale for 960x600 window

        // Create character layer (20 columns x 12 rows with 16x16 font = 320x192 pixels)
        textLayer_ = std::make_shared<Engine::CharacterLayer>(20, 12, font_);

        // Setup C64-inspired palette
        std::array<Engine::Color, 256> palette;
        palette[0] = Engine::Color{21, 21, 59, 255};       // Dark blue background
        palette[1] = Engine::Color{120, 105, 196, 255};    // Light blue text (default)
        palette[2] = Engine::Color{255, 255, 255, 255};    // White
        palette[3] = Engine::Color{170, 255, 238, 255};    // Cyan
        palette[4] = Engine::Color{238, 238, 119, 255};    // Yellow
        palette[5] = Engine::Color{0, 204, 85, 255};       // Green
        palette[6] = Engine::Color{221, 136, 85, 255};     // Orange
        palette[7] = Engine::Color{255, 119, 119, 255};    // Light Red

        // Fill remaining with grayscale
        for (int i = 8; i < 256; ++i) {
            uint8_t gray = static_cast<uint8_t>(i);
            palette[i] = Engine::Color{gray, gray, gray, 255};
        }

        screen_->setPalette(palette);

        // Add to first layer
        auto& layers = getEngine()->getLayers();
        if (!layers.empty()) {
            layers[0]->addIndexedPixelBuffer(screen_);
        }

        // Print initial text
        textLayer_->setColor(2);  // White
        textLayer_->print("*** COMMODORE 64 BASIC V2 ***\n\n");

        textLayer_->setColor(3);  // Cyan
        textLayer_->print(" 64K RAM SYSTEM  38911 BASIC BYTES FREE\n\n");

        textLayer_->setColor(1);  // Light blue
        textLayer_->print("READY.\n");

        LOG_INFO("Terminal demo initialized!");
    }

    void update(float deltaTime) override {
        time_ += deltaTime;

        // Type message character by character
        if (!messageComplete_ && time_ >= nextCharTime_) {
            if (messageIndex_ < demoMessage_.length()) {
                textLayer_->print(demoMessage_[messageIndex_]);
                messageIndex_++;
                nextCharTime_ = time_ + charDelay_;
            } else {
                messageComplete_ = true;
            }
        }

        // Animate position with a gentle sine wave
        float offsetX = std::sin(time_ * 0.5f) * 20.0f;
        float offsetY = std::cos(time_ * 0.3f) * 15.0f;
        screen_->setPosition(Engine::Vec2{offsetX, offsetY});

        // Render text layer to screen
        screen_->clear(0);  // Clear to background color
        textLayer_->render(*screen_);
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
    std::shared_ptr<Engine::CharacterLayer> textLayer_;
    Engine::PixelFontPtr font_;
    float time_ = 0.0f;

    // Typing simulation
    std::string demoMessage_ = "10 PRINT \"HELLO WORLD\"\n"
                               "20 FOR I=1 TO 10\n"
                               "30 PRINT \"RETRO COMPUTING\";\n"
                               "40 NEXT I\n"
                               "RUN\n";
    size_t messageIndex_ = 0;
    float charDelay_ = 0.1f;  // 100ms between characters
    float nextCharTime_ = 1.0f;  // Start after 1 second
    bool messageComplete_ = false;
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
                      << " | Character Layer Demo" << std::endl;
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
    config.title = "Character Layer Demo";
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
    auto terminal = std::make_shared<TerminalDemo>();
    auto fpsDisplay = std::make_shared<FPSDisplay>();

    engine.attachGameObject(terminal);
    engine.attachGameObject(fpsDisplay);

    // Info text
    std::cout << "\n=== Character Layer Demo ===\n";
    std::cout << "Text mode character display!\n";
    std::cout << "  - 20x12 character grid\n";
    std::cout << "  - Virtual cursor\n";
    std::cout << "  - Automatic scrolling\n";
    std::cout << "  - Per-character colors\n";
    std::cout << "  - Animated position offset\n";
    std::cout << "\nControls:\n";
    std::cout << "  ESC - Quit\n\n";

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

    std::cout << "Character layer demo shut down successfully!\n";
    return 0;
}
