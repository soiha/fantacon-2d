#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/IUpdateable.h"
#include "engine/IndexedPixelBuffer.h"
#include "engine/GLRenderer.h"
#include "engine/Layer.h"
#include "engine/FPSCounter.h"
#include "engine/Text.h"
#include "engine/Logger.h"
#include <SDL.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>

// GameObject that demonstrates indexed color with palette animation
class PaletteAnimationDemo : public Engine::GameObject,
                              public Engine::IUpdateable {
public:
    PaletteAnimationDemo() : GameObject("PaletteAnimationDemo") {}

    void onAttached() override {
        LOG_INFO("PaletteAnimationDemo attached!");

        // Create indexed pixel buffer (320x240 for retro feel)
        indexedBuffer_ = std::make_shared<Engine::IndexedPixelBuffer>(320, 240);
        indexedBuffer_->setPosition(Engine::Vec2{240, 180});  // Center it
        indexedBuffer_->setScale(1.0f);

        // Initialize palette with a fire/plasma gradient
        std::array<Engine::Color, 256> palette;
        for (int i = 0; i < 256; ++i) {
            float t = i / 255.0f;

            // Create a fire-like palette gradient
            if (i < 64) {
                // Black to dark red
                float brightness = i / 64.0f;
                palette[i] = Engine::Color{
                    static_cast<uint8_t>(brightness * 64),
                    0,
                    0,
                    255
                };
            } else if (i < 128) {
                // Dark red to orange
                float t2 = (i - 64) / 64.0f;
                palette[i] = Engine::Color{
                    static_cast<uint8_t>(64 + t2 * 191),
                    static_cast<uint8_t>(t2 * 128),
                    0,
                    255
                };
            } else if (i < 192) {
                // Orange to yellow
                float t2 = (i - 128) / 64.0f;
                palette[i] = Engine::Color{
                    255,
                    static_cast<uint8_t>(128 + t2 * 127),
                    static_cast<uint8_t>(t2 * 128),
                    255
                };
            } else {
                // Yellow to white
                float t2 = (i - 192) / 63.0f;
                palette[i] = Engine::Color{
                    255,
                    255,
                    static_cast<uint8_t>(128 + t2 * 127),
                    255
                };
            }
        }
        indexedBuffer_->setPalette(palette);

        // Draw an interesting pattern - plasma-like effect using palette indices
        for (int y = 0; y < 240; ++y) {
            for (int x = 0; x < 320; ++x) {
                // Create plasma pattern using sine waves
                float fx = x / 320.0f;
                float fy = y / 240.0f;

                float v1 = std::sin(fx * 10.0f + fy * 10.0f);
                float v2 = std::sin(std::sqrt(fx*fx + fy*fy) * 20.0f);
                float v3 = std::sin(fx * 15.0f);
                float v4 = std::sin(fy * 15.0f);

                float plasma = (v1 + v2 + v3 + v4) / 4.0f;

                // Map to palette index (0-255)
                uint8_t index = static_cast<uint8_t>((plasma + 1.0f) * 127.5f);
                indexedBuffer_->setPixel(x, y, index);
            }
        }

        // Add to first layer
        auto& layers = getEngine()->getLayers();
        if (!layers.empty()) {
            layers[0]->addIndexedPixelBuffer(indexedBuffer_);
        }

        LOG_INFO("Created 320x240 indexed pixel buffer with fire palette!");
    }

    void update(float deltaTime) override {
        // Animate the palette by rotating colors - this is the key performance test!
        // With shader-based rendering, this only uploads 1KB (256 colors)
        // With CPU rendering, this would re-convert all 76,800 pixels!

        paletteTime_ += deltaTime * paletteSpeed_;

        // Get current palette
        auto currentPalette = indexedBuffer_->getPalette();

        // Rotate palette entries to create color cycling effect
        std::array<Engine::Color, 256> newPalette;
        int shift = static_cast<int>(paletteTime_) % 256;

        for (int i = 0; i < 256; ++i) {
            newPalette[i] = currentPalette[(i + shift) % 256];
        }

        // Update palette - with GLRenderer this is super fast!
        indexedBuffer_->setPalette(newPalette);
    }

    void onDestroy() override {
        if (indexedBuffer_) {
            auto& layers = getEngine()->getLayers();
            if (!layers.empty()) {
                layers[0]->removeIndexedPixelBuffer(indexedBuffer_);
            }
        }
    }

private:
    std::shared_ptr<Engine::IndexedPixelBuffer> indexedBuffer_;
    float paletteTime_ = 0.0f;
    float paletteSpeed_ = 60.0f;  // Palette rotation speed
};

// Simple FPS counter that prints to console
class FPSDisplay : public Engine::GameObject,
                   public Engine::IUpdateable {
public:
    FPSDisplay() : GameObject("FPSDisplay"), fpsCounter_(60) {}

    void update(float deltaTime) override {
        fpsCounter_.update();

        timeSinceUpdate_ += deltaTime;
        if (timeSinceUpdate_ >= 1.0f) {
            std::cout << "FPS: " << std::fixed << std::setprecision(1)
                      << fpsCounter_.getFPS() << " | Palette Animation (Shader-based)" << std::endl;
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
    config.title = "Indexed Color Palette Animation Demo";
    config.width = 800;
    config.height = 600;
    config.logLevel = Engine::LogLevel::Info;
    config.logToFile = false;
    config.showTimestamp = true;
    config.showWallClock = true;
    config.showFrame = true;
    config.showLogLevel = true;

    // Create engine with GLRenderer for shader-based palette rendering
    Engine::Engine engine;

    // IMPORTANT: Use GLRenderer instead of SDLRenderer for shader performance!
    auto glRenderer = std::make_unique<Engine::GLRenderer>();
    if (!glRenderer->init(config.title, config.width, config.height)) {
        LOG_ERROR("Failed to initialize GLRenderer!");
        return 1;
    }

    if (!engine.init(config, std::move(glRenderer))) {
        LOG_ERROR("Failed to initialize engine!");
        return 1;
    }

    // Create a layer for the indexed pixel buffer
    auto layer = engine.createLayer(0);

    // Create and attach GameObjects
    auto paletteDemo = std::make_shared<PaletteAnimationDemo>();
    auto fpsDisplay = std::make_shared<FPSDisplay>();

    engine.attachGameObject(paletteDemo);
    engine.attachGameObject(fpsDisplay);

    // Info text
    std::cout << "\n=== Indexed Color Palette Animation Demo ===" << std::endl;
    std::cout << "This demo showcases shader-based palette rendering:" << std::endl;
    std::cout << "  - 320x240 indexed pixel buffer (76,800 pixels)" << std::endl;
    std::cout << "  - 256-color fire palette" << std::endl;
    std::cout << "  - Real-time palette cycling animation" << std::endl;
    std::cout << "\nPerformance Benefits:" << std::endl;
    std::cout << "  CPU-based: Would re-convert ALL 76,800 pixels every frame (~300KB upload)" << std::endl;
    std::cout << "  GPU shader: Only uploads palette (256 colors = 1KB upload)" << std::endl;
    std::cout << "  Result: ~300x bandwidth reduction!" << std::endl;
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

    std::cout << "Palette animation demo shut down successfully!" << std::endl;
    return 0;
}
