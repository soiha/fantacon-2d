#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/IUpdateable.h"
#include "engine/Layer.h"
#include "engine/IndexedPixelBuffer.h"
#include "engine/AttributedTextGrid.h"
#include "engine/Palette.h"
#include "engine/Mesh3D.h"
#include "engine/ResourceManager.h"
#include "engine/VulkanRenderer.h"
#include "engine/FPSCounter.h"
#include "engine/Logger.h"
#include <SDL.h>
#include <cmath>

// Demo combining Mesh3D and AttributedTextGrid with layer transparency
class LayerTransparencyDemo : public Engine::GameObject,
                               public Engine::IUpdateable {
public:
    LayerTransparencyDemo() : GameObject("LayerTransparencyDemo") {}

    void onAttached() override {
        LOG_INFO("LayerTransparencyDemo attached!");

        // ========== LAYER 0: 3D Meshes (background) ==========
        // Set up mesh palette with vibrant colors
        Engine::Palette meshPalette;
        meshPalette.setColor(0, {0, 0, 0});       // Background
        meshPalette.setColor(1, {255, 64, 64});   // Red - Bright
        meshPalette.setColor(2, {64, 255, 64});   // Green - Bright
        meshPalette.setColor(3, {64, 64, 255});   // Blue - Bright
        meshPalette.setColor(4, {255, 255, 64});  // Yellow - Bright
        meshPalette.setColor(5, {255, 64, 255});  // Magenta - Bright
        meshPalette.setColor(6, {64, 255, 255});  // Cyan - Bright
        meshPalette.setColor(7, {255, 128, 64});  // Orange - Bright
        meshPalette.setColor(8,  {153, 38, 38});  // Red - Medium
        meshPalette.setColor(9,  {38, 153, 38});  // Green - Medium
        meshPalette.setColor(10, {38, 38, 153});  // Blue - Medium
        meshPalette.setColor(11, {153, 153, 38}); // Yellow - Medium
        meshPalette.setColor(12, {153, 38, 153}); // Magenta - Medium
        meshPalette.setColor(13, {38, 153, 153}); // Cyan - Medium
        meshPalette.setColor(14, {153, 76, 38});  // Orange - Medium

        // Create mesh layer with 0.5 opacity
        meshLayer_ = std::make_shared<Engine::Layer>(0);
        meshLayer_->setOpacity(0.5f);  // 50% transparent!

        // Create three rotating 3D objects
        cube_ = Engine::Mesh3D::createCube(50.0f);
        cube_->setPosition(Engine::Vec2{240, 300});
        cube_->setAutoRotation(Engine::Vec3{0.5f, 0.8f, 0.3f});
        cube_->setCameraDistance(200.0f);
        cube_->setFOV(60.0f);
        cube_->getBuffer()->setPalette(meshPalette.getColors());
        meshLayer_->addAttachable(cube_);

        pyramid_ = Engine::Mesh3D::createPyramid(60.0f);
        pyramid_->setPosition(Engine::Vec2{480, 300});
        pyramid_->setAutoRotation(Engine::Vec3{0.7f, 0.5f, 0.9f});
        pyramid_->setCameraDistance(200.0f);
        pyramid_->setFOV(60.0f);
        pyramid_->getBuffer()->setPalette(meshPalette.getColors());
        meshLayer_->addAttachable(pyramid_);

        sphere_ = Engine::Mesh3D::createSphere(40.0f, 12);
        sphere_->setPosition(Engine::Vec2{720, 300});
        sphere_->setAutoRotation(Engine::Vec3{0.6f, 0.9f, 0.4f});
        sphere_->setCameraDistance(200.0f);
        sphere_->setFOV(60.0f);
        sphere_->getBuffer()->setPalette(meshPalette.getColors());
        meshLayer_->addAttachable(sphere_);

        // ========== LAYER 1: Text Grid (foreground) ==========
        // Set up VGA-style palette for text
        Engine::Palette textPalette;
        textPalette.setColor(0, {0, 0, 0});         // Black (transparent background)
        textPalette.setColor(1, {0, 0, 170});       // Blue
        textPalette.setColor(2, {0, 170, 0});       // Green
        textPalette.setColor(3, {0, 170, 170});     // Cyan
        textPalette.setColor(4, {170, 0, 0});       // Red
        textPalette.setColor(5, {170, 0, 170});     // Magenta
        textPalette.setColor(6, {170, 85, 0});      // Brown
        textPalette.setColor(7, {170, 170, 170});   // Light Gray
        textPalette.setColor(8, {85, 85, 85});      // Dark Gray
        textPalette.setColor(9, {85, 85, 255});     // Bright Blue
        textPalette.setColor(10, {85, 255, 85});    // Bright Green
        textPalette.setColor(11, {85, 255, 255});   // Bright Cyan
        textPalette.setColor(12, {255, 85, 85});    // Bright Red
        textPalette.setColor(13, {255, 85, 255});   // Bright Magenta
        textPalette.setColor(14, {255, 255, 85});   // Yellow
        textPalette.setColor(15, {255, 255, 255});  // White

        // Create text layer with 0.5 opacity
        textLayer_ = std::make_shared<Engine::Layer>(1);
        textLayer_->setOpacity(0.5f);  // 50% transparent!

        // Load ATASCII font
        auto& resourceMgr = getEngine()->getResourceManager();
        font_ = resourceMgr.loadPixelFont(
            "atascii",
            "examples/resources/atascii.gif",
            16, 16, 16, "", 0, 256
        );

        if (!font_) {
            LOG_ERROR("Failed to load font!");
            return;
        }

        // Create text grid
        textGrid_ = std::make_shared<Engine::AttributedTextGrid>(font_, 60, 37);
        textGrid_->setPosition(Engine::Vec2{0, 0});
        textGrid_->getBuffer()->setPalette(textPalette.getColors());

        // Define text attributes
        textGrid_->setAttributeDef(0, 7, 0);   // Light gray on black
        textGrid_->setAttributeDef(1, 15, 1);  // White on blue (header)
        textGrid_->setAttributeDef(2, 14, 0);  // Yellow on black
        textGrid_->setAttributeDef(3, 10, 0);  // Bright green on black
        textGrid_->setAttributeDef(4, 12, 0);  // Bright red on black
        textGrid_->setAttributeDef(5, 11, 0);  // Bright cyan on black

        // Draw content
        drawTextContent();

        // Add text grid to layer
        textLayer_->addAttachable(textGrid_);

        // Add layers to the engine
        getEngine()->addLayer(meshLayer_);
        getEngine()->addLayer(textLayer_);

        LOG_INFO("Created two layers with 0.5 opacity each!");
        LOG_INFO("Layer 0: 3D Meshes (Cube, Pyramid, Sphere)");
        LOG_INFO("Layer 1: VGA Text Grid");
        LOG_INFO("Both layers blend together with alpha transparency!");
    }

    void drawTextContent() {
        textGrid_->clear(' ', 0);

        // Title bar
        textGrid_->fill(0, 0, 60, 1, ' ', 1);
        textGrid_->print(2, 0, " LAYER TRANSPARENCY DEMO ", 1);
        textGrid_->print(35, 0, "[ESC:QUIT] [SPACE:TOGGLE]", 1);

        // Main content
        textGrid_->print(2, 3, "LAYER TRANSPARENCY TEST", 2);
        textGrid_->print(2, 5, "Two layers with 50% opacity:", 3);
        textGrid_->print(4, 7, "Layer 0: 3D Meshes (background)", 0);
        textGrid_->print(4, 8, "Layer 1: Text Grid (foreground)", 0);

        textGrid_->print(2, 11, "You can see THROUGH both layers!", 5);
        textGrid_->print(2, 12, "The meshes show through the text", 5);
        textGrid_->print(2, 13, "The text shows through the meshes", 5);

        // Instructions box
        textGrid_->print(2, 16, "+--------------------------------------------+", 2);
        textGrid_->print(2, 17, "|  CONTROLS:                                 |", 2);
        textGrid_->print(2, 18, "|  SPACE - Toggle layer opacity (cycle)     |", 2);
        textGrid_->print(2, 19, "|  1     - Both layers 100% opaque           |", 2);
        textGrid_->print(2, 20, "|  2     - Both layers 50% transparent       |", 2);
        textGrid_->print(2, 21, "|  3     - Both layers 25% transparent       |", 2);
        textGrid_->print(2, 22, "|  ESC   - Quit                              |", 2);
        textGrid_->print(2, 23, "+--------------------------------------------+", 2);

        // Status display (will be updated in update())
        textGrid_->print(2, 26, "Current opacity mode:", 3);

        // Footer
        textGrid_->fill(0, 36, 60, 1, ' ', 1);
        textGrid_->print(2, 36, "Hardware-accelerated alpha blending via Vulkan", 1);
    }

    void update(float deltaTime) override {
        time_ += deltaTime;

        // Handle input for opacity toggling
        auto& input = getEngine()->getInput();
        if (input.isKeyPressed(SDLK_SPACE)) {
            opacityMode_ = (opacityMode_ + 1) % 3;
            updateOpacity();
        }
        if (input.isKeyPressed(SDLK_1)) {
            opacityMode_ = 0;
            updateOpacity();
        }
        if (input.isKeyPressed(SDLK_2)) {
            opacityMode_ = 1;
            updateOpacity();
        }
        if (input.isKeyPressed(SDLK_3)) {
            opacityMode_ = 2;
            updateOpacity();
        }

        // Update mesh animations
        cube_->update(deltaTime);
        pyramid_->update(deltaTime);
        sphere_->update(deltaTime);

        // Add bobbing motion to pyramid
        float bobAmount = std::sin(time_ * 2.0f) * 30.0f;
        pyramid_->setPosition(Engine::Vec2{480, 300 + bobAmount});

        // Pulsate sphere
        float scaleAmount = 1.0f + std::sin(time_ * 3.0f) * 0.3f;
        sphere_->setScale(scaleAmount);

        // Update status text
        const char* modeText[] = {
            "MODE 1: Both layers 100% opaque      ",
            "MODE 2: Both layers 50% transparent  ",
            "MODE 3: Both layers 25% transparent  "
        };
        textGrid_->print(2, 28, modeText[opacityMode_], 4);

        // Animated status
        int frame = static_cast<int>(time_ * 10) % 20;
        char animText[60];
        snprintf(animText, sizeof(animText), "Frame: %3d | Time: %.1fs", frame, time_);
        textGrid_->print(2, 30, animText, 0);

        // Blinking indicator
        bool blink = (static_cast<int>(time_ * 2) % 2) == 0;
        if (blink) {
            textGrid_->print(2, 32, ">>> TRANSPARENCY ACTIVE <<<", 5);
        } else {
            textGrid_->print(2, 32, "                           ", 0);
        }

        // No manual rendering needed - the layers handle it!
    }

    void updateOpacity() {
        float opacities[] = {1.0f, 0.5f, 0.25f};
        float opacity = opacities[opacityMode_];

        meshLayer_->setOpacity(opacity);
        textLayer_->setOpacity(opacity);

        LOG_INFO_FMT("Layer opacity set to: %.2f", opacity);
    }

    void onDestroy() override {
        // Cleanup handled by shared_ptr
    }

private:
    // Mesh layer
    std::shared_ptr<Engine::Layer> meshLayer_;
    std::shared_ptr<Engine::Mesh3D> cube_;
    std::shared_ptr<Engine::Mesh3D> pyramid_;
    std::shared_ptr<Engine::Mesh3D> sphere_;

    // Text layer
    std::shared_ptr<Engine::Layer> textLayer_;
    std::shared_ptr<Engine::PixelFont> font_;
    std::shared_ptr<Engine::AttributedTextGrid> textGrid_;

    float time_ = 0.0f;
    int opacityMode_ = 1;  // Start with 50% opacity
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
            LOG_INFO_FMT("FPS: %.1f | Layer Transparency Demo", fpsCounter_.getFPS());
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
    config.title = "Layer Transparency Demo - Alpha Blending";
    config.width = 960;
    config.height = 600;
    config.logLevel = Engine::LogLevel::Info;
    config.logToFile = false;
    config.showTimestamp = true;
    config.showWallClock = true;
    config.showFrame = true;
    config.showLogLevel = true;

    // Create engine with VulkanRenderer
    Engine::Engine engine;

    auto vulkanRenderer = std::make_unique<Engine::VulkanRenderer>();
    // Note: Don't call init() here - engine.init() will do it!

    if (!engine.init(config, std::move(vulkanRenderer))) {
        LOG_ERROR("Failed to initialize engine!");
        return 1;
    }

    // Create demo GameObject
    auto demo = std::make_shared<LayerTransparencyDemo>();
    auto fpsDisplay = std::make_shared<FPSDisplay>();

    engine.attachGameObject(demo);
    engine.attachGameObject(fpsDisplay);

    // Info text
    LOG_INFO("\n=== Layer Transparency Demo ===");
    LOG_INFO("Demonstrating layer opacity with alpha blending!");
    LOG_INFO("Two layers rendering on top of each other:");
    LOG_INFO("  Layer 0: 3D Meshes (Cube, Pyramid, Sphere)");
    LOG_INFO("  Layer 1: VGA Text Grid");
    LOG_INFO("\nBoth layers at 50% opacity - you can see through them!");
    LOG_INFO("\nControls:");
    LOG_INFO("  SPACE - Toggle opacity mode (100%, 50%, 25%)");
    LOG_INFO("  1     - Set both layers to 100% opaque");
    LOG_INFO("  2     - Set both layers to 50% transparent");
    LOG_INFO("  3     - Set both layers to 25% transparent");
    LOG_INFO("  ESC   - Quit\n");

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

    LOG_INFO("Layer transparency demo shut down successfully!");
    return 0;
}
