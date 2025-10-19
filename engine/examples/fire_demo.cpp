#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/IUpdateable.h"
#include "engine/IndexedPixelBuffer.h"
#include "engine/GLRenderer.h"
#include "engine/Palette.h"
#include "engine/Layer.h"
#include "engine/FPSCounter.h"
#include "engine/Logger.h"
#include <SDL.h>
#include <iostream>
#include <random>
#include <cmath>

// GameObject that renders classic fire effect
class FireEffect : public Engine::GameObject,
                   public Engine::IUpdateable {
public:
    FireEffect() : GameObject("FireEffect"), rng_(std::random_device{}()), dist_(0, 255) {}

    void onAttached() override {
        LOG_INFO("FireEffect attached!");

        // Create a chunky pixel buffer - low res for that retro look!
        fire_ = std::make_shared<Engine::IndexedPixelBuffer>(160, 120);
        fire_->setPosition(Engine::Vec2{240, 180});  // Center it
        fire_->setScale(3.0f);  // 3x upscale for chunky pixels

        // Apply fire gradient palette (black -> red -> orange -> yellow -> white)
        auto firePalette = Engine::Palette::createFireGradient();
        fire_->setPalette(firePalette.getColors());

        // Clear to black initially
        fire_->clear(0);

        // Add to first layer
        auto& layers = getEngine()->getLayers();
        if (!layers.empty()) {
            layers[0]->addIndexedPixelBuffer(fire_);
        }

        LOG_INFO("Created 160x120 fire effect with fire gradient palette!");
    }

    void update(float deltaTime) override {
        int width = fire_->getWidth();
        int height = fire_->getHeight();

        // Add random "heat sources" at the bottom row
        for (int x = 0; x < width; ++x) {
            // Random heat intensity at bottom
            uint8_t heat = dist_(rng_);
            fire_->setPixel(x, height - 1, heat);
        }

        // Propagate fire upward with cooling
        for (int y = 0; y < height - 1; ++y) {
            for (int x = 0; x < width; ++x) {
                // Sample pixels below and around current position
                int sum = 0;
                int count = 0;

                // Pixel directly below
                if (y + 1 < height) {
                    sum += fire_->getPixel(x, y + 1);
                    count++;
                }

                // Pixel below-left
                if (x > 0 && y + 1 < height) {
                    sum += fire_->getPixel(x - 1, y + 1);
                    count++;
                }

                // Pixel below-right
                if (x < width - 1 && y + 1 < height) {
                    sum += fire_->getPixel(x + 1, y + 1);
                    count++;
                }

                // Pixel two rows below for more vertical spread
                if (y + 2 < height) {
                    sum += fire_->getPixel(x, y + 2);
                    count++;
                }

                if (count > 0) {
                    // Average and cool down
                    int average = sum / count;

                    // Cooling factor - flames get cooler as they rise
                    int cooling = 3 + (dist_(rng_) & 7);  // Random 3-10
                    int newHeat = std::max(0, average - cooling);

                    fire_->setPixel(x, y, static_cast<uint8_t>(newHeat));
                } else {
                    fire_->setPixel(x, y, 0);
                }
            }
        }
    }

    void onDestroy() override {
        if (fire_) {
            auto& layers = getEngine()->getLayers();
            if (!layers.empty()) {
                layers[0]->removeIndexedPixelBuffer(fire_);
            }
        }
    }

private:
    std::shared_ptr<Engine::IndexedPixelBuffer> fire_;
    std::mt19937 rng_;
    std::uniform_int_distribution<int> dist_;
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
                      << " | Old-School Fire Effect" << std::endl;
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
    config.title = "Old-School Fire Effect";
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

    auto glRenderer = std::make_unique<Engine::GLRenderer>();
    if (!glRenderer->init(config.title, config.width, config.height)) {
        LOG_ERROR("Failed to initialize GLRenderer!");
        return 1;
    }

    if (!engine.init(config, std::move(glRenderer))) {
        LOG_ERROR("Failed to initialize engine!");
        return 1;
    }

    // Create a layer for the fire effect
    auto layer = engine.createLayer(0);

    // Create and attach GameObjects
    auto fireEffect = std::make_shared<FireEffect>();
    auto fpsDisplay = std::make_shared<FPSDisplay>();

    engine.attachGameObject(fireEffect);
    engine.attachGameObject(fpsDisplay);

    // Info text
    std::cout << "\n=== Old-School Fire Effect Demo ===" << std::endl;
    std::cout << "Classic demoscene fire effect using indexed color!" << std::endl;
    std::cout << "  - 160x120 indexed pixel buffer" << std::endl;
    std::cout << "  - 256-color fire gradient palette" << std::endl;
    std::cout << "  - Real-time procedural fire animation" << std::endl;
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

    std::cout << "Fire effect demo shut down successfully!" << std::endl;
    return 0;
}
