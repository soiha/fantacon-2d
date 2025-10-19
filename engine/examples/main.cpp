#include "engine/Engine.h"
#include "engine/Sprite.h"
#include "engine/Tilemap.h"
#include "engine/Texture.h"
#include "engine/FPSCounter.h"
#include "engine/Text.h"
#include <SDL.h>
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>

int main(int argc, char* argv[]) {
    // Create and initialize engine
    Engine::Engine engine;
    if (!engine.init("2D Engine Demo", 800, 600)) {
        std::cerr << "Failed to initialize engine!" << std::endl;
        return 1;
    }

    // load tex
    auto texture = std::make_shared<Engine::Texture>();
    texture->loadFromFile("/Users/soiha/src/2d/engine/examples/resources/hg.png", engine.getRenderer());

    // Create layers with different render orders
    auto backgroundLayer = engine.createLayer(0);   // Renders first
    auto gameLayer = engine.createLayer(10);        // Renders second
    auto uiLayer = engine.createLayer(20);          // Renders last (on top)

    // EXAMPLE 1: Create a tilemap for the background layer
    // This demonstrates how you would use tilemaps with a tileset
    auto tilemap = std::make_shared<Engine::Tilemap>(20, 15, 32, 32);
    tilemap->setPosition(Engine::Vec2{0, 0});

    // Fill with a checkerboard pattern (you would normally load a tileset texture)
    for (int y = 0; y < tilemap->getHeight(); ++y) {
        for (int x = 0; x < tilemap->getWidth(); ++x) {
            tilemap->setTile(x, y, (x + y) % 2);
        }
    }

    // Note: To actually render the tilemap, you would need to:
    // 1. Create a tileset texture: auto tileset = std::make_shared<Texture>();
    // 2. Load it: tileset->loadFromFile("tileset.png", engine.getRenderer());
    // 3. Set it: tilemap->setTileset(tileset, tilesPerRow);
    // For now, we skip adding it since we don't have actual image files
    // backgroundLayer->addTilemap(tilemap);

    // EXAMPLE 2: Create sprites for the game layer
    // Note: In a real game, you would load actual image files
    // For this example, we'll create sprite objects but they won't render without textures

    // Example sprite 1: Player character
    auto player = std::make_shared<Engine::Sprite>(texture, Engine::Vec2{64, 64});
    player->setPosition(Engine::Vec2{400, 300});  // Center of screen
    player->setScale(1.0f);           // 2x size
    player->setRotation(0);
    gameLayer->addSprite(player);

    // Example sprite 2: Rotating object
    auto rotatingSprite = std::make_shared<Engine::Sprite>(texture,  Engine::Vec2{64, 64});
    rotatingSprite->setPosition(Engine::Vec2{200, 200});
    rotatingSprite->setScale(Engine::Vec2{1.0f, 1.0f});
    gameLayer->addSprite(rotatingSprite);

    // Example sprite 3: UI element on top layer
    auto uiElement = std::make_shared<Engine::Sprite>();
    uiElement->setPosition(Engine::Vec2{10, 10});
    uiElement->setScale(1.0f);
    uiLayer->addSprite(uiElement);

    // FPS Counter and Text
    Engine::FPSCounter fpsCounter(60);  // Average over 60 frames
    Engine::Text fpsText;

    // Load font via ResourceManager
    auto& rm = engine.getResourceManager();
    auto font = rm.loadFont("helvetica-24", "/System/Library/Fonts/Helvetica.ttc", 24);
    if (!font) {
        std::cerr << "Warning: Could not load font. FPS will not be displayed." << std::endl;
    } else {
        fpsText.setFont(font);
    }

    // Main game loop
    std::cout << "Engine initialized successfully!" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Arrow Keys - Move player sprite" << std::endl;
    std::cout << "  R - Rotate sprite" << std::endl;
    std::cout << "  + / - - Scale sprite" << std::endl;
    std::cout << "  ESC - Quit" << std::endl;
    std::cout << std::endl;
    std::cout << "Note: Sprites won't be visible without loaded textures." << std::endl;
    std::cout << "This demo shows the engine structure and API usage." << std::endl;

    bool quit = false;
    SDL_Event e;
    float rotation = 0.0f;
    float scale = 1.0f;

    while (!quit && engine.isRunning()) {
        // Update FPS counter
        fpsCounter.update();

        // Update FPS text every frame
        std::ostringstream fpsStream;
        fpsStream << "FPS: " << std::fixed << std::setprecision(1) << fpsCounter.getFPS();
        fpsText.setText(fpsStream.str(), engine.getRenderer(), Engine::Color{0, 255, 0, 255});

        // Handle events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        quit = true;
                        break;
                    case SDLK_UP:
                        player->setPosition(Engine::Vec2{
                            player->getPosition().x,
                            player->getPosition().y - 5
                        });
                        break;
                    case SDLK_DOWN:
                        player->setPosition(Engine::Vec2{
                            player->getPosition().x,
                            player->getPosition().y + 5
                        });
                        break;
                    case SDLK_LEFT:
                        player->setPosition(Engine::Vec2{
                            player->getPosition().x - 5,
                            player->getPosition().y
                        });
                        break;
                    case SDLK_RIGHT:
                        player->setPosition(Engine::Vec2{
                            player->getPosition().x + 5,
                            player->getPosition().y
                        });
                        break;
                    case SDLK_r:
                        rotation += 15.0f;
                        player->setRotation(rotation);
                        break;
                    case SDLK_EQUALS:  // '+' key
                        scale += 0.1f;
                        player->setScale(scale);
                        break;
                    case SDLK_MINUS:
                        scale = std::max(0.1f, scale - 0.1f);
                        player->setScale(scale);
                        break;
                }
            }
        }

        // Update rotating sprite
        rotation += 1.0f;
        if (rotation >= 360.0f) rotation = 0.0f;
        rotatingSprite->setRotation(rotation);

        // Start rendering frame
        engine.getRenderer().clear();

        // Render all layers (background -> game -> ui)
        for (const auto& layer : engine.getLayers()) {
            layer->render(engine.getRenderer());
        }

        // Render FPS text on top of everything
        if (fpsText.isValid()) {
            engine.getRenderer().renderText(fpsText, Engine::Vec2{10, 10});
        }

        // Present the final frame
        engine.getRenderer().present();
    }

    // Cleanup
    engine.shutdown();

    std::cout << "Engine shut down successfully!" << std::endl;
    return 0;
}
