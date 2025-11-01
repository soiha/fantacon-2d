#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/IUpdateable.h"
#include "engine/Layer.h"
#include "engine/IndexedPixelBuffer.h"
#include "engine/Palette.h"
#include "engine/Mesh3D.h"
#include "engine/ResourceManager.h"
#include "engine/GLRenderer.h"
#include "engine/FPSCounter.h"
#include "engine/Logger.h"
#include <SDL.h>
#include <memory>

// Demo showing normal-based back-face culling
class MeshNormalsDemo : public Engine::GameObject,
                        public Engine::IUpdateable {
public:
    MeshNormalsDemo() : GameObject("MeshNormalsDemo") {}

    void onAttached() override {
        LOG_INFO("MeshNormalsDemo attached!");

        // Set up the palette
        Engine::Palette palette;
        palette.setColor(0, {0, 0, 0});       // Background
        palette.setColor(1, {255, 64, 64});   // Red
        palette.setColor(2, {64, 255, 64});   // Green
        palette.setColor(3, {64, 64, 255});   // Blue
        palette.setColor(4, {255, 255, 64});  // Yellow
        palette.setColor(5, {255, 64, 255});  // Magenta
        palette.setColor(6, {64, 255, 255});  // Cyan
        palette.setColor(7, {255, 128, 64});  // Orange

        // Get the layer to attach meshes to
        auto& layers = getEngine()->getLayers();
        if (layers.empty()) {
            LOG_ERROR("No layers available!");
            return;
        }
        layer_ = layers[0];

        // Load mesh with normals from OBJ file
        auto& resourceMgr = getEngine()->getResourceManager();
        loadedCube_ = resourceMgr.loadMesh3D("cube_normals", "examples/resources/cube_with_normals.obj");

        if (loadedCube_) {
            loadedCube_->setPosition(Engine::Vec2{240, 300});
            loadedCube_->setAutoRotation(Engine::Vec3{0.0f, 0.5f, 0.0f});
            loadedCube_->setFOV(60.0f);
            loadedCube_->setScale(2.0f);
            loadedCube_->getBuffer()->setPalette(palette.getColors());
            layer_->addAttachable(loadedCube_);

            LOG_INFO_FMT("Left: Cube loaded from OBJ with %zu normals", loadedCube_->getNormals().size());
        } else {
            LOG_ERROR("Warning: Failed to load cube_with_normals.obj");
        }

        // Create cube using factory method (no normals, calculated on-the-fly)
        factoryCube_ = Engine::Mesh3D::createCube(40.0f);
        factoryCube_->setPosition(Engine::Vec2{720, 300});
        factoryCube_->setAutoRotation(Engine::Vec3{0.0f, 0.5f, 0.0f});
        factoryCube_->setFOV(60.0f);
        factoryCube_->setScale(2.0f);
        factoryCube_->getBuffer()->setPalette(palette.getColors());
        layer_->addAttachable(factoryCube_);

        LOG_INFO_FMT("Right: Cube created with factory (%zu normals - calculated)", factoryCube_->getNormals().size());
        LOG_INFO("Both cubes should look identical!");
        LOG_INFO("Left uses pre-loaded normals, right calculates normals from vertices.");
    }

    void update(float deltaTime) override {
        // Update meshes
        if (loadedCube_) {
            loadedCube_->update(deltaTime);
        }
        factoryCube_->update(deltaTime);

        // No manual rendering needed - the layer handles it!
    }

private:
    std::shared_ptr<Engine::Layer> layer_;
    std::shared_ptr<Engine::Mesh3D> loadedCube_;
    std::shared_ptr<Engine::Mesh3D> factoryCube_;
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
            LOG_INFO_FMT("FPS: %.1f | Mesh Normal Demo", fpsCounter_.getFPS());
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
    config.title = "Mesh3D Normal Demo - Loaded vs Calculated Normals";
    config.width = 960;
    config.height = 600;
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
    auto demo = std::make_shared<MeshNormalsDemo>();
    auto fpsDisplay = std::make_shared<FPSDisplay>();

    engine.attachGameObject(demo);
    engine.attachGameObject(fpsDisplay);

    // Info text
    LOG_INFO("\n=== Mesh3D Normal-Based Back-Face Culling Demo ===");
    LOG_INFO("Demonstrating loaded normals vs calculated normals");
    LOG_INFO("Features:");
    LOG_INFO("  - Normal-based back-face culling");
    LOG_INFO("  - Automatic fallback to calculated normals");
    LOG_INFO("  - Both methods produce the same visual result");
    LOG_INFO("\nControls:");
    LOG_INFO("  ESC - Quit\n");

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

    LOG_INFO("Mesh Normal demo shut down successfully!");
    return 0;
}
