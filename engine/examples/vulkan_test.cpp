#include "engine/Engine.h"
#include "engine/VulkanRenderer.h"
#include "engine/GameObject.h"
#include "engine/IUpdateable.h"
#include "engine/Logger.h"
#include <SDL.h>

class VulkanTestGame : public Engine::GameObject, public Engine::IUpdateable {
public:
    VulkanTestGame() : GameObject("VulkanTestGame") {}

    void onAttached() override {
        LOG_INFO("Vulkan Test Demo Started!");
        LOG_INFO("You should see a black window - this means Vulkan is working!");
        LOG_INFO("Press ESC to quit");
    }

    void update(float deltaTime) override {
        // Nothing to update yet
    }
};

int main(int argc, char* argv[]) {
    Engine::Engine engine;
    Engine::EngineConfig config;
    config.title = "Vulkan Test - Black Screen";
    config.width = 800;
    config.height = 600;

    // Create Vulkan renderer
    auto vulkanRenderer = std::make_unique<Engine::VulkanRenderer>();
    if (!vulkanRenderer->init(config.title, config.width, config.height)) {
        LOG_ERROR("Failed to initialize Vulkan renderer!");
        return 1;
    }

    if (!engine.init(config, std::move(vulkanRenderer))) {
        LOG_ERROR("Failed to initialize engine!");
        return 1;
    }

    // Create and attach test game object
    auto game = std::make_shared<VulkanTestGame>();
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
        engine.render();
    }

    engine.shutdown();

    LOG_INFO("Vulkan test completed successfully!");
    return 0;
}
