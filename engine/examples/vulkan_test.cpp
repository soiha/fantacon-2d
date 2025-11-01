#include "engine/Engine.h"
#include "engine/VulkanRenderer.h"
#include "engine/GameObject.h"
#include "engine/IUpdateable.h"
#include "engine/Sprite.h"
#include "engine/Layer.h"
#include "engine/Logger.h"
#include <SDL.h>
#include <vector>

class VulkanTestGame : public Engine::GameObject, public Engine::IUpdateable {
public:
    VulkanTestGame(Engine::IRenderer& renderer)
        : GameObject("VulkanTestGame"), renderer_(renderer) {}

    void onAttached() override {
        LOG_INFO("Vulkan Sprite Test Started!");
        LOG_INFO("Creating a colorful gradient sprite...");

        // Create a procedural gradient texture (256x256)
        const int texWidth = 256;
        const int texHeight = 256;
        std::vector<Engine::Color> pixels(texWidth * texHeight);

        // Generate a colorful gradient
        for (int y = 0; y < texHeight; y++) {
            for (int x = 0; x < texWidth; x++) {
                int index = y * texWidth + x;
                pixels[index].r = static_cast<uint8_t>((x * 255) / texWidth);       // Red gradient left-to-right
                pixels[index].g = static_cast<uint8_t>((y * 255) / texHeight);      // Green gradient top-to-bottom
                pixels[index].b = static_cast<uint8_t>(((x + y) * 128) / (texWidth + texHeight)); // Blue diagonal
                pixels[index].a = 255;  // Fully opaque
            }
        }

        // Create streaming texture
        texture_ = renderer_.createStreamingTexture(texWidth, texHeight);
        if (!texture_) {
            LOG_ERROR("Failed to create streaming texture!");
            return;
        }

        // Upload pixel data
        renderer_.updateTexture(*texture_, pixels.data(), texWidth, texHeight);
        LOG_INFO("Texture created and uploaded to GPU");

        // Create sprite
        sprite_ = std::make_shared<Engine::Sprite>(texture_, Engine::Vec2{300, 200});
        sprite_->setScale(1.5f);  // Make it a bit bigger

        // Create a layer and add the sprite
        layer_ = std::make_shared<Engine::Layer>();
        layer_->addSprite(sprite_);

        LOG_INFO("Sprite created at position (300, 200) with 1.5x scale");
        LOG_INFO("You should see a colorful gradient square!");
        LOG_INFO("Press ESC to quit");
    }

    void update(float deltaTime) override {
        // Slowly rotate the sprite to show animation
        if (sprite_) {
            float currentRotation = sprite_->getRotation();
            sprite_->setRotation(currentRotation + 30.0f * deltaTime);  // 30 degrees per second
        }
    }

    void render(Engine::IRenderer& renderer) {
        if (layer_) {
            layer_->render(renderer);
        }
    }

private:
    Engine::IRenderer& renderer_;
    Engine::TexturePtr texture_;
    std::shared_ptr<Engine::Sprite> sprite_;
    std::shared_ptr<Engine::Layer> layer_;
};

int main(int argc, char* argv[]) {
    Engine::Engine engine;
    Engine::EngineConfig config;
    config.title = "Vulkan Sprite Test - Rotating Gradient";
    config.width = 800;
    config.height = 600;

    // Create Vulkan renderer
    auto vulkanRenderer = std::make_unique<Engine::VulkanRenderer>();
    if (!vulkanRenderer->init(config.title, config.width, config.height)) {
        LOG_ERROR("Failed to initialize Vulkan renderer!");
        return 1;
    }

    // Store renderer reference before moving
    Engine::IRenderer* rendererPtr = vulkanRenderer.get();

    if (!engine.init(config, std::move(vulkanRenderer))) {
        LOG_ERROR("Failed to initialize engine!");
        return 1;
    }

    // Create and attach test game object (pass renderer reference)
    auto game = std::make_shared<VulkanTestGame>(*rendererPtr);
    engine.attachGameObject(game);

    // Main loop
    while (engine.isRunning()) {
        if (!engine.pollEvents()) {
            break;
        }

        if (engine.getInput().isKeyPressed(SDLK_ESCAPE)) {
            break;
        }

        engine.update();

        // Render the game's sprite
        engine.getRenderer().clear();
        game->render(engine.getRenderer());
        engine.getRenderer().present();
    }

    engine.shutdown();

    LOG_INFO("Vulkan sprite test completed successfully!");
    return 0;
}
