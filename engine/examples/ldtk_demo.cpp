#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/IUpdateable.h"
#include "engine/IInputHandler.h"
#include "engine/IRenderable.h"
#include "engine/Logger.h"
#include <SDL.h>
#include <iostream>
#include <cmath>

/*
 * LDtk Integration Demo
 *
 * This demo shows how to:
 * 1. Load a tilemap from an LDtk file
 * 2. Add it to a layer for rendering
 * 3. Query tiles for collision detection
 * 4. Convert between world and grid coordinates
 *
 * To use this demo, you'll need to create an LDtk file:
 *
 * 1. Download LDtk from https://ldtk.io
 * 2. Create a new project
 * 3. Add a tileset (any tile image will work)
 * 4. Create a level called "Level_0"
 * 5. Add a tile layer called "Ground"
 * 6. Paint some tiles
 * 7. Save the project as: examples/resources/world.ldtk
 * 8. Make sure the tileset image is also in examples/resources/
 *
 * Expected file structure:
 *   examples/resources/
 *     world.ldtk         <- Your LDtk project file
 *     tiles.png          <- Your tileset image (or whatever you named it)
 */

// GameObject that animates layer offset with a sine wave
class LayerOffsetAnimator : public Engine::GameObject,
                             public Engine::IUpdateable {
public:
    LayerOffsetAnimator() : GameObject("LayerOffsetAnimator") {}

    void onAttached() override {
        // Get the first layer from the engine
        auto& layers = getEngine()->getLayers();
        if (!layers.empty()) {
            layer_ = layers[0];
            LOG_INFO("LayerOffsetAnimator attached! Layer will scroll in a sine wave pattern.");
        }
    }

    void update(float deltaTime) override {
        if (!layer_) return;

        // Update time
        time_ += deltaTime * speed_;

        // Calculate sine wave offset
        // Oscillate horizontally to show the full width of the tilemap
        float offsetX = std::sin(time_) * amplitude_;
        float offsetY = 0.0f;  // Could also add vertical movement if desired

        // Set the layer offset
        layer_->setOffset(Engine::Vec2{offsetX, offsetY});

        // Log offset periodically
        timeSinceLog_ += deltaTime;
        if (timeSinceLog_ >= 2.0f) {
            LOG_INFO_FMT("Layer offset: (%.1f, %.1f)", offsetX, offsetY);
            timeSinceLog_ = 0.0f;
        }
    }

private:
    std::shared_ptr<Engine::Layer> layer_;
    float time_ = 0.0f;
    float speed_ = 0.5f;      // Oscillation speed (radians per second)
    float amplitude_ = 300.0f; // Horizontal scroll distance in pixels
    float timeSinceLog_ = 0.0f;
};

// Example GameObject that can walk on tiles and detect collisions
class TileWalker : public Engine::GameObject,
                   public Engine::IUpdateable,
                   public Engine::IInputHandler,
                   public Engine::IRenderable {
public:
    TileWalker() : GameObject("TileWalker") {}

    void onAttached() override {
        LOG_INFO("TileWalker attached! Use arrow keys to move.");
        LOG_INFO("The walker will check which tile it's standing on.");

        // Try to get the tilemap from ResourceManager
        tilemap_ = getEngine()->getResourceManager().getTilemap("level_ground");
    }

    void handleInput(const Engine::Input& input) override {
        velocity_ = {0, 0};

        if (input.isKeyHeld(SDLK_LEFT))  velocity_.x = -speed_;
        if (input.isKeyHeld(SDLK_RIGHT)) velocity_.x = speed_;
        if (input.isKeyHeld(SDLK_UP))    velocity_.y = -speed_;
        if (input.isKeyHeld(SDLK_DOWN))  velocity_.y = speed_;

        if (input.isKeyPressed(SDLK_SPACE)) {
            queryCurrentTile();
        }
    }

    void update(float deltaTime) override {
        position_.x += velocity_.x * deltaTime;
        position_.y += velocity_.y * deltaTime;

        // Keep in bounds
        position_.x = std::max(0.0f, std::min(800.0f, position_.x));
        position_.y = std::max(0.0f, std::min(600.0f, position_.y));
    }

    void render(Engine::IRenderer& renderer) override {
        // Draw a simple rectangle to represent the walker
        // In a real game, you'd use a sprite here
        // For now, this shows where the walker is
        // (We'd need to add a rectangle drawing method to IRenderer for this to work)
    }

    int getRenderOrder() const override { return 100; } // Render on top of tilemap

private:
    void queryCurrentTile() {
        if (!tilemap_) {
            LOG_WARNING("No tilemap loaded, cannot query tiles");
            return;
        }

        // Get the tile at the walker's current position
        int tileId = tilemap_->getTileAtWorldPos(position_);

        // Convert position to grid coordinates
        Engine::Vec2 gridPos = tilemap_->worldToGrid(position_);
        int gridX = static_cast<int>(gridPos.x);
        int gridY = static_cast<int>(gridPos.y);

        LOG_INFO_FMT("Walker at world position (%.1f, %.1f)", position_.x, position_.y);
        LOG_INFO_FMT("  -> Grid position (%d, %d)", gridX, gridY);
        LOG_INFO_FMT("  -> Tile ID: %d %s", tileId, tileId >= 0 ? "" : "(empty)");

        // Example collision logic
        if (tileId >= 0) {
            LOG_INFO("Standing on a tile!");
        } else {
            LOG_INFO("Standing on empty space");
        }
    }

    Engine::Vec2 position_{400, 300};
    Engine::Vec2 velocity_{0, 0};
    float speed_ = 200.0f;
    std::shared_ptr<Engine::Tilemap> tilemap_;
};

int main(int argc, char* argv[]) {
    // Configure engine
    Engine::EngineConfig config;
    config.title = "LDtk Integration Demo";
    config.width = 800;
    config.height = 600;
    config.logLevel = Engine::LogLevel::Info;
    config.logToFile = false;
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

    // Create a layer for the tilemap
    auto layer = engine.createLayer(0);

    // Load tilemap from LDtk
    auto& rm = engine.getResourceManager();

    std::cout << "\n=== LDtk Integration Demo ===" << std::endl;
    std::cout << "Attempting to load LDtk tilemap from: examples/resources/world.ldtk" << std::endl;
    std::cout << "  Level: Level_0" << std::endl;
    std::cout << "  Layer: Ground" << std::endl;
    std::cout << "\nIf loading fails, you'll need to create an LDtk file first." << std::endl;
    std::cout << "See the comments at the top of ldtk_demo.cpp for instructions.\n" << std::endl;

    auto tilemap = rm.loadLDtkTilemap(
        "level_ground",                    // Cache name
        "../examples/resources/world.ldtk", // Path to LDtk file (relative to build dir)
        "Level_0",                         // Level identifier in LDtk
        "Ground"                           // Layer identifier in LDtk
    );

    if (tilemap) {
        LOG_INFO("LDtk tilemap loaded successfully!");

        // Add tilemap to layer for rendering
        layer->addTilemap(tilemap);

        LOG_INFO_FMT("Tilemap dimensions: %dx%d tiles",
                     tilemap->getWidth(), tilemap->getHeight());
        LOG_INFO_FMT("Tile size: %dx%d pixels",
                     tilemap->getTileWidth(), tilemap->getTileHeight());

        // Example: Query a specific tile
        int centerTile = tilemap->getTile(tilemap->getWidth() / 2, tilemap->getHeight() / 2);
        LOG_INFO_FMT("Center tile ID: %d", centerTile);

        // Example: Convert grid to world coordinates
        Engine::Vec2 worldPos = tilemap->gridToWorld(5, 5);
        LOG_INFO_FMT("Grid (5, 5) -> World (%.1f, %.1f)", worldPos.x, worldPos.y);

        // Create layer offset animator (scrolls the layer in a sine wave)
        auto animator = std::make_shared<LayerOffsetAnimator>();
        engine.attachGameObject(animator);

        // Create walker GameObject
        auto walker = std::make_shared<TileWalker>();
        engine.attachGameObject(walker);

        std::cout << "\n=== Controls ===" << std::endl;
        std::cout << "  Arrow Keys - Move the walker" << std::endl;
        std::cout << "  Space      - Query tile at current position" << std::endl;
        std::cout << "  ESC        - Quit" << std::endl;
        std::cout << "\nThe tilemap layer will automatically scroll in a sine wave pattern" << std::endl;
        std::cout << "to showcase the layer offset feature and show the full map extent.\n" << std::endl;

    } else {
        LOG_ERROR("Failed to load LDtk tilemap!");
        std::cout << "\n*** LDtk file not found or invalid ***" << std::endl;
        std::cout << "To create a test LDtk file:" << std::endl;
        std::cout << "1. Download LDtk from https://ldtk.io" << std::endl;
        std::cout << "2. Create a new project" << std::endl;
        std::cout << "3. Add a tileset" << std::endl;
        std::cout << "4. Create a level called 'Level_0'" << std::endl;
        std::cout << "5. Add a tile layer called 'Ground'" << std::endl;
        std::cout << "6. Paint some tiles" << std::endl;
        std::cout << "7. Save as: examples/resources/world.ldtk" << std::endl;
        std::cout << "\nThe engine will continue running to show the expected workflow.\n" << std::endl;
    }

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

    // Cleanup
    engine.shutdown();

    std::cout << "LDtk demo shut down successfully!" << std::endl;
    return 0;
}
