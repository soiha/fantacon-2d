#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/IUpdateable.h"
#include "engine/Layer.h"
#include "engine/IndexedPixelBuffer.h"
#include "engine/Palette.h"
#include "engine/Mesh3D.h"
#include "engine/GLRenderer.h"
#include "engine/FPSCounter.h"
#include "engine/Logger.h"
#include <SDL.h>
#include <cmath>

// Demo showing Mesh3D as first-class renderables attached to layers
class MeshDemo : public Engine::GameObject,
                 public Engine::IUpdateable {
public:
    MeshDemo() : GameObject("MeshDemo") {}

    void onAttached() override {
        LOG_INFO("MeshDemo attached!");

        // Set up the palette with vibrant colors for our meshes
        // We'll create 3 brightness levels for each color (bright, medium, dark)
        Engine::Palette palette;
        palette.setColor(0, {0, 0, 0});       // Background

        // Base colors (indices 1-7) - Full brightness
        palette.setColor(1, {255, 64, 64});   // Red - Bright
        palette.setColor(2, {64, 255, 64});   // Green - Bright
        palette.setColor(3, {64, 64, 255});   // Blue - Bright
        palette.setColor(4, {255, 255, 64});  // Yellow - Bright
        palette.setColor(5, {255, 64, 255});  // Magenta - Bright
        palette.setColor(6, {64, 255, 255});  // Cyan - Bright
        palette.setColor(7, {255, 128, 64});  // Orange - Bright

        // Medium brightness (indices 8-14) - 60% brightness
        palette.setColor(8,  {153, 38, 38});  // Red - Medium
        palette.setColor(9,  {38, 153, 38});  // Green - Medium
        palette.setColor(10, {38, 38, 153});  // Blue - Medium
        palette.setColor(11, {153, 153, 38}); // Yellow - Medium
        palette.setColor(12, {153, 38, 153}); // Magenta - Medium
        palette.setColor(13, {38, 153, 153}); // Cyan - Medium
        palette.setColor(14, {153, 76, 38});  // Orange - Medium

        // Dark (indices 15-21) - 30% brightness
        palette.setColor(15, {76, 19, 19});   // Red - Dark
        palette.setColor(16, {19, 76, 19});   // Green - Dark
        palette.setColor(17, {19, 19, 76});   // Blue - Dark
        palette.setColor(18, {76, 76, 19});   // Yellow - Dark
        palette.setColor(19, {76, 19, 76});   // Magenta - Dark
        palette.setColor(20, {19, 76, 76});   // Cyan - Dark
        palette.setColor(21, {76, 38, 19});   // Orange - Dark

        // Get the layer to attach meshes to
        auto& layers = getEngine()->getLayers();
        if (layers.empty()) {
            LOG_ERROR("No layers available!");
            return;
        }
        layer_ = layers[0];

        // Create three different 3D objects using factory methods

        // Cube on the left
        cube_ = Engine::Mesh3D::createCube(40.0f);
        cube_->setPosition(Engine::Vec2{240, 300});
        cube_->setAutoRotation(Engine::Vec3{0.5f, 0.8f, 0.3f});
        cube_->setCameraDistance(200.0f);
        cube_->setFOV(60.0f);  // 60 degrees - good balance of perspective
        cube_->getBuffer()->setPalette(palette.getColors());
        layer_->addAttachable(cube_);

        // Pyramid in the middle
        pyramid_ = Engine::Mesh3D::createPyramid(50.0f);
        pyramid_->setPosition(Engine::Vec2{480, 300});
        pyramid_->setAutoRotation(Engine::Vec3{0.7f, 0.5f, 0.9f});
        pyramid_->setCameraDistance(200.0f);
        pyramid_->setFOV(60.0f);  // 60 degrees
        pyramid_->getBuffer()->setPalette(palette.getColors());
        layer_->addAttachable(pyramid_);

        // Sphere on the right
        sphere_ = Engine::Mesh3D::createSphere(35.0f, 12);
        sphere_->setPosition(Engine::Vec2{720, 300});
        sphere_->setAutoRotation(Engine::Vec3{0.6f, 0.9f, 0.4f});
        sphere_->setCameraDistance(200.0f);
        sphere_->setFOV(60.0f);  // 60 degrees
        sphere_->getBuffer()->setPalette(palette.getColors());
        layer_->addAttachable(sphere_);

        // Add wireframe versions below to demonstrate render modes
        // Wireframe cube on the left
        wireCube_ = Engine::Mesh3D::createCube(40.0f);
        wireCube_->setPosition(Engine::Vec2{240, 450});
        wireCube_->setAutoRotation(Engine::Vec3{0.5f, 0.8f, 0.3f});
        wireCube_->setCameraDistance(200.0f);
        wireCube_->setFOV(60.0f);
        wireCube_->setRenderMode(Engine::MeshRenderMode::Wireframe);
        wireCube_->getBuffer()->setPalette(palette.getColors());
        layer_->addAttachable(wireCube_);

        // Wireframe pyramid in the middle
        wirePyramid_ = Engine::Mesh3D::createPyramid(50.0f);
        wirePyramid_->setPosition(Engine::Vec2{480, 450});
        wirePyramid_->setAutoRotation(Engine::Vec3{0.7f, 0.5f, 0.9f});
        wirePyramid_->setCameraDistance(200.0f);
        wirePyramid_->setFOV(60.0f);
        wirePyramid_->setRenderMode(Engine::MeshRenderMode::Wireframe);
        wirePyramid_->getBuffer()->setPalette(palette.getColors());
        layer_->addAttachable(wirePyramid_);

        // Wireframe sphere on the right
        wireSphere_ = Engine::Mesh3D::createSphere(35.0f, 12);
        wireSphere_->setPosition(Engine::Vec2{720, 450});
        wireSphere_->setAutoRotation(Engine::Vec3{0.6f, 0.9f, 0.4f});
        wireSphere_->setCameraDistance(200.0f);
        wireSphere_->setFOV(60.0f);
        wireSphere_->setRenderMode(Engine::MeshRenderMode::Wireframe);
        wireSphere_->getBuffer()->setPalette(palette.getColors());
        layer_->addAttachable(wireSphere_);

        LOG_INFO("Created Mesh3D objects: filled and wireframe versions!");
    }

    void update(float deltaTime) override {
        time_ += deltaTime;

        // Update all filled meshes (applies auto-rotation)
        cube_->update(deltaTime);
        pyramid_->update(deltaTime);
        sphere_->update(deltaTime);

        // Add gentle up-down motion to pyramids
        float bobAmount = std::sin(time_ * 2.0f) * 30.0f;
        pyramid_->setPosition(Engine::Vec2{480, 300 + bobAmount});
        wirePyramid_->setPosition(Engine::Vec2{480, 450 + bobAmount});

        // Pulsate the spheres' scale
        float scaleAmount = 1.0f + std::sin(time_ * 3.0f) * 0.3f;
        sphere_->setScale(scaleAmount);
        wireSphere_->setScale(scaleAmount);

        // Update all wireframe meshes
        wireCube_->update(deltaTime);
        wirePyramid_->update(deltaTime);
        wireSphere_->update(deltaTime);

        // No manual rendering needed - the layer handles it!
    }

    void onDestroy() override {
        // Cleanup handled automatically by layer
    }

private:
    std::shared_ptr<Engine::Layer> layer_;
    std::shared_ptr<Engine::Mesh3D> cube_;
    std::shared_ptr<Engine::Mesh3D> pyramid_;
    std::shared_ptr<Engine::Mesh3D> sphere_;
    std::shared_ptr<Engine::Mesh3D> wireCube_;
    std::shared_ptr<Engine::Mesh3D> wirePyramid_;
    std::shared_ptr<Engine::Mesh3D> wireSphere_;
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
            LOG_INFO_FMT("FPS: %.1f | Mesh3D Demo", fpsCounter_.getFPS());
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
    config.title = "Mesh3D Demo - First-Class Renderables";
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

    // Create a layer for the meshes
    auto layer = engine.createLayer(0);

    // Create and attach GameObjects
    auto meshDemo = std::make_shared<MeshDemo>();
    auto fpsDisplay = std::make_shared<FPSDisplay>();

    engine.attachGameObject(meshDemo);
    engine.attachGameObject(fpsDisplay);

    // Info text
    LOG_INFO("\n=== Mesh3D First-Class Renderables Demo ===");
    LOG_INFO("Meshes are now first-class citizens like Sprites!");
    LOG_INFO("Features demonstrated:");
    LOG_INFO("  - Factory methods: createCube(), createPyramid(), createSphere()");
    LOG_INFO("  - Attached to layers with addMesh3D()");
    LOG_INFO("  - Auto-rotation support");
    LOG_INFO("  - Perspective projection with FOV control (60°)");
    LOG_INFO("  - Back-face culling (hidden line removal)");
    LOG_INFO("  - Normal-based lighting with colored shading");
    LOG_INFO("  - Filled triangle rendering (top row)");
    LOG_INFO("  - Wireframe rendering with lighting (bottom row)");
    LOG_INFO("  - Transform properties (position, rotation, scale)");
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

    LOG_INFO("Mesh3D demo shut down successfully!");
    return 0;
}
