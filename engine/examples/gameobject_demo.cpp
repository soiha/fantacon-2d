#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/IUpdateable.h"
#include "engine/IInputHandler.h"
#include "engine/IRenderable.h"
#include "engine/ICollidable.h"
#include "engine/IOnDebug.h"
#include "engine/Sprite.h"
#include "engine/Layer.h"
#include "engine/Texture.h"
#include "engine/FPSCounter.h"
#include "engine/Text.h"
#include "engine/Logger.h"
#include <SDL.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>

// Example GameObject: A bouncing ball with physics
class BouncingBall : public Engine::GameObject,
                     public Engine::IUpdateable,
                     public Engine::IOnDebug {
public:
    BouncingBall(const std::string& name, Engine::Vec2 position, float scale = 1.0f)
        : GameObject(name), position_(position), scale_(scale) {}

    void onAttached() override {
        LOG_INFO_STREAM(getName() << " attached to engine!");

        // Load texture from ResourceManager
        auto& rm = getEngine()->getResourceManager();
        auto texture = rm.getTexture("hg_sprite");

        if (texture) {
            // Create sprite with texture
            sprite_ = std::make_shared<Engine::Sprite>(texture);
            sprite_->setPosition(position_);
            sprite_->setScale(scale_);

            // Add sprite to first layer for rendering
            auto& layers = getEngine()->getLayers();
            if (!layers.empty()) {
                layers[0]->addSprite(sprite_);
            }
        }
    }

    void onDestroy() override {
        // Remove sprite from layer
        if (sprite_) {
            auto& layers = getEngine()->getLayers();
            if (!layers.empty()) {
                layers[0]->removeSprite(sprite_);
            }
        }
    }

    void update(float deltaTime) override {
        // Apply gravity
        velocity_.y += gravity_ * deltaTime;

        // Update position
        position_.x += velocity_.x * deltaTime;
        position_.y += velocity_.y * deltaTime;

        // Bounce off walls (accounting for sprite size)
        float halfWidth = (sprite_ ? sprite_->getTexture()->getWidth() * scale_ / 2 : 20.0f);
        float halfHeight = (sprite_ ? sprite_->getTexture()->getHeight() * scale_ / 2 : 20.0f);

        if (position_.x - halfWidth < 0) {
            position_.x = halfWidth;
            velocity_.x = std::abs(velocity_.x) * bounceFactor_;
        }
        if (position_.x + halfWidth > 800) {
            position_.x = 800 - halfWidth;
            velocity_.x = -std::abs(velocity_.x) * bounceFactor_;
        }
        if (position_.y + halfHeight > 600) {
            position_.y = 600 - halfHeight;
            velocity_.y = -std::abs(velocity_.y) * bounceFactor_;
        }

        // Dampen velocity
        velocity_.x *= 0.99f;

        // Update sprite position
        if (sprite_) {
            sprite_->setPosition(position_);
            sprite_->setRotation(sprite_->getRotation() + 100.0f * deltaTime);  // Spin!
        }
    }

    void onDebug(Engine::IRenderer& renderer) override {
        // Debug visualization would go here
        // In a real implementation, you'd draw the circle outline
    }

    Engine::Vec2 getPosition() const { return position_; }

private:
    std::shared_ptr<Engine::Sprite> sprite_;
    Engine::Vec2 position_{400, 300};
    Engine::Vec2 velocity_{150, -300};
    float scale_ = 1.0f;
    float gravity_ = 500.0f;
    float bounceFactor_ = 0.8f;
};

// Example GameObject: Player controlled with keyboard
class Player : public Engine::GameObject,
               public Engine::IUpdateable,
               public Engine::IInputHandler {
public:
    Player() : GameObject("Player") {}

    void onAttached() override {
        std::cout << "Player attached! Use arrow keys to move." << std::endl;
    }

    void handleInput(const Engine::Input& input) override {
        inputVelocity_ = {0, 0};

        if (input.isKeyHeld(SDLK_LEFT)) {
            inputVelocity_.x = -speed_;
        }
        if (input.isKeyHeld(SDLK_RIGHT)) {
            inputVelocity_.x = speed_;
        }
        if (input.isKeyHeld(SDLK_UP)) {
            inputVelocity_.y = -speed_;
        }
        if (input.isKeyHeld(SDLK_DOWN)) {
            inputVelocity_.y = speed_;
        }

        if (input.isKeyPressed(SDLK_SPACE)) {
            LOG_INFO_FMT("Player position: (%.1f, %.1f)", position_.x, position_.y);
        }
    }

    void update(float deltaTime) override {
        position_.x += inputVelocity_.x * deltaTime;
        position_.y += inputVelocity_.y * deltaTime;

        // Keep in bounds
        position_.x = std::max(0.0f, std::min(800.0f, position_.x));
        position_.y = std::max(0.0f, std::min(600.0f, position_.y));
    }

    Engine::Vec2 getPosition() const { return position_; }

private:
    Engine::Vec2 position_{400, 300};
    Engine::Vec2 inputVelocity_{0, 0};
    float speed_ = 200.0f;
};

// Example: FPS Display as a GameObject
class FPSDisplay : public Engine::GameObject,
                   public Engine::IUpdateable,
                   public Engine::IRenderable {
public:
    FPSDisplay() : GameObject("FPSDisplay"), fpsCounter_(60) {}

    void onAttached() override {
        // Load font from ResourceManager
        auto& rm = getEngine()->getResourceManager();
        auto font = rm.getFont("helvetica-24");
        if (font) {
            fpsText_.setFont(font);
            fpsText_.setText("FPS: 0.0 | GameObjects: 0", getEngine()->getRenderer(), Engine::Color{0, 255, 0, 255});
        }
    }

    void update(float deltaTime) override {
        fpsCounter_.update();

        // Update text every 100ms
        timeSinceUpdate_ += deltaTime;
        if (timeSinceUpdate_ >= 0.1f) {
            std::ostringstream stream;
            stream << "FPS: " << std::fixed << std::setprecision(1) << fpsCounter_.getFPS();
            stream << " | GameObjects: " << getEngine()->getGameObjects().size();
            fpsText_.setText(stream.str(), getEngine()->getRenderer(), Engine::Color{0, 255, 0, 255});
            timeSinceUpdate_ = 0.0f;
        }
    }

    void render(Engine::IRenderer& renderer) override {
        if (fpsText_.isValid()) {
            renderer.renderText(fpsText_, Engine::Vec2{10, 10});
        }
    }

    int getRenderOrder() const override { return 1000; } // Render last (on top)

private:
    Engine::FPSCounter fpsCounter_;
    Engine::Text fpsText_;
    float timeSinceUpdate_ = 0.0f;
};

// Example: Fixed timestep physics object
class PhysicsObject : public Engine::GameObject,
                      public Engine::IFixedUpdateable {
public:
    PhysicsObject() : GameObject("PhysicsObject") {
        position_ = {100, 100};
    }

    void fixedUpdate(float fixedDeltaTime) override {
        // Simulate physics at fixed 60Hz
        angle_ += rotationSpeed_ * fixedDeltaTime;
        if (angle_ >= 360.0f) angle_ -= 360.0f;

        // Orbit in a circle
        position_.x = 400 + std::cos(angle_ * 3.14159f / 180.0f) * orbitRadius_;
        position_.y = 300 + std::sin(angle_ * 3.14159f / 180.0f) * orbitRadius_;
    }

    Engine::Vec2 getPosition() const { return position_; }

private:
    Engine::Vec2 position_;
    float angle_ = 0.0f;
    float rotationSpeed_ = 90.0f;  // degrees per second
    float orbitRadius_ = 150.0f;
};

// Example: Layer scrolling demonstration
class LayerScroller : public Engine::GameObject,
                       public Engine::IUpdateable {
public:
    LayerScroller() : GameObject("LayerScroller") {}

    void update(float deltaTime) override {
        auto& layers = getEngine()->getLayers();
        if (layers.empty()) return;

        // Scroll the main layer based on arrow key input
        const auto& input = getEngine()->getInput();
        Engine::Vec2 scroll{0, 0};

        if (input.isKeyHeld(SDLK_LEFT)) scroll.x += scrollSpeed_ * deltaTime;
        if (input.isKeyHeld(SDLK_RIGHT)) scroll.x -= scrollSpeed_ * deltaTime;
        if (input.isKeyHeld(SDLK_UP)) scroll.y += scrollSpeed_ * deltaTime;
        if (input.isKeyHeld(SDLK_DOWN)) scroll.y -= scrollSpeed_ * deltaTime;

        if (scroll.x != 0 || scroll.y != 0) {
            layers[0]->moveOffset(scroll);
        }
    }

private:
    float scrollSpeed_ = 100.0f;
};

// Example: Sprite anchoring demonstration
class AnchoredSprites : public Engine::GameObject,
                        public Engine::IUpdateable {
public:
    AnchoredSprites() : GameObject("AnchoredSprites") {}

    void onAttached() override {
        auto& rm = getEngine()->getResourceManager();
        auto texture = rm.getTexture("hg_sprite");
        if (!texture) return;

        auto& layers = getEngine()->getLayers();
        if (layers.empty()) return;

        // Create parent sprite in center of screen
        parentSprite_ = std::make_shared<Engine::Sprite>(texture);
        parentSprite_->setPosition(Engine::Vec2{400, 300});
        parentSprite_->setScale(0.8f);
        layers[0]->addSprite(parentSprite_);

        // Create child sprites anchored to parent's corners and center
        auto createChild = [&](Engine::AnchorPoint parentAnchor, Engine::AnchorPoint childAnchor,
                              Engine::Vec2 offset, float scale) {
            auto child = std::make_shared<Engine::Sprite>(texture);
            child->setScale(scale);
            child->setParent(parentSprite_, parentAnchor, childAnchor, offset);
            layers[0]->addSprite(child);
            childSprites_.push_back(child);
        };

        // Anchor small sprites to each corner of parent (child's center to parent's corners)
        createChild(Engine::AnchorPoint::TopLeft, Engine::AnchorPoint::Center, {0, 0}, 0.2f);
        createChild(Engine::AnchorPoint::TopRight, Engine::AnchorPoint::Center, {0, 0}, 0.2f);
        createChild(Engine::AnchorPoint::BottomLeft, Engine::AnchorPoint::Center, {0, 0}, 0.2f);
        createChild(Engine::AnchorPoint::BottomRight, Engine::AnchorPoint::Center, {0, 0}, 0.2f);

        // Anchor one to center with an offset
        createChild(Engine::AnchorPoint::Center, Engine::AnchorPoint::Center, {50, -50}, 0.3f);
    }

    void update(float deltaTime) override {
        // Rotate parent - children will follow automatically
        if (parentSprite_) {
            parentSprite_->setRotation(parentSprite_->getRotation() + 30.0f * deltaTime);
        }
    }

    void onDestroy() override {
        auto& layers = getEngine()->getLayers();
        if (!layers.empty()) {
            if (parentSprite_) layers[0]->removeSprite(parentSprite_);
            for (auto& child : childSprites_) {
                layers[0]->removeSprite(child);
            }
        }
    }

private:
    std::shared_ptr<Engine::Sprite> parentSprite_;
    std::vector<std::shared_ptr<Engine::Sprite>> childSprites_;
};

int main(int argc, char* argv[]) {
    // Configure engine with logging
    Engine::EngineConfig config;
    config.title = "GameObject Demo";
    config.width = 800;
    config.height = 600;
    config.logLevel = Engine::LogLevel::Info;  // Change to Debug to see more
    config.logToFile = false;  // Set to true to log to file
    config.showTimestamp = true;
    config.showWallClock = true;
    config.showFrame = true;
    config.showLogLevel = true;

    // Create and initialize engine
    Engine::Engine engine;
    if (!engine.init(config)) {
        LOG_ERROR("Failed to initialize engine!");
        return 1;
    }

    // Create a layer for sprites
    auto layer = engine.createLayer(0);

    // Load resources via ResourceManager
    auto& rm = engine.getResourceManager();
    auto hgTexture = rm.loadTexture("hg_sprite", "../examples/resources/hg.png");
    if (!hgTexture) {
        std::cerr << "Warning: Could not load hg.png sprite!" << std::endl;
    }

    // Load font via ResourceManager (for FPS display)
    auto font = rm.loadFont("helvetica-24", "/System/Library/Fonts/Helvetica.ttc", 24);
    if (!font) {
        std::cerr << "Warning: Could not load font." << std::endl;
    }

    // Create and attach GameObjects
    auto ball1 = std::make_shared<BouncingBall>("Ball1", Engine::Vec2{200, 100}, 0.5f);
    auto ball2 = std::make_shared<BouncingBall>("Ball2", Engine::Vec2{600, 150}, 0.3f);
    auto player = std::make_shared<Player>();
    auto physicsObj = std::make_shared<PhysicsObject>();
    auto anchoredSprites = std::make_shared<AnchoredSprites>();
    auto layerScroller = std::make_shared<LayerScroller>();
    auto fpsDisplay = std::make_shared<FPSDisplay>();

    engine.attachGameObject(ball1);
    engine.attachGameObject(ball2);
    engine.attachGameObject(player);
    engine.attachGameObject(physicsObj);
    engine.attachGameObject(anchoredSprites);
    engine.attachGameObject(layerScroller);
    engine.attachGameObject(fpsDisplay);

    // Demonstrate printf-style logging
    int totalGameObjects = engine.getGameObjects().size();
    LOG_INFO_FMT("Attached %d GameObjects to the engine", totalGameObjects);

    // Info text
    std::cout << "\n=== GameObject System Demo ===" << std::endl;
    std::cout << "This demo shows the GameObject system with:" << std::endl;
    std::cout << "  - IUpdateable: Bouncing balls with physics" << std::endl;
    std::cout << "  - IInputHandler: Player controlled with arrow keys" << std::endl;
    std::cout << "  - IFixedUpdateable: Physics object orbiting at fixed 60Hz" << std::endl;
    std::cout << "  - Sprite Anchoring: Parent sprite with anchored children" << std::endl;
    std::cout << "  - Layer Offsets: Arrow keys scroll the entire layer" << std::endl;
    std::cout << "  - IRenderable: FPS display as a GameObject" << std::endl;
    std::cout << "\nControls:" << std::endl;
    std::cout << "  Arrow Keys - Scroll the layer (camera movement)" << std::endl;
    std::cout << "  Space - Print player position" << std::endl;
    std::cout << "  D - Toggle debug mode" << std::endl;
    std::cout << "  ESC - Quit\n" << std::endl;

    while (engine.isRunning()) {
        // Poll events (handles input processing, returns false on quit)
        if (!engine.pollEvents()) {
            break;
        }

        // Check for ESC key or 'D' key for debug toggle
        if (engine.getInput().isKeyPressed(SDLK_ESCAPE)) {
            break;
        }
        if (engine.getInput().isKeyPressed(SDLK_d)) {
            engine.setDebugMode(!engine.isDebugMode());
            LOG_INFO_FMT("Debug mode: %s", engine.isDebugMode() ? "ON" : "OFF");
        }

        // Update engine (automatically calculates deltaTime)
        engine.update();

        // Render (handles clear, layers, IRenderables, present)
        engine.render();
    }

    // Cleanup
    engine.shutdown();

    std::cout << "GameObject demo shut down successfully!" << std::endl;
    return 0;
}
