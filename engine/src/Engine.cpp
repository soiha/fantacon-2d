#include "engine/Engine.h"
#include "engine/SDLRenderer.h"
#include "engine/IUpdateable.h"
#include "engine/IInputHandler.h"
#include "engine/IRenderable.h"
#include "engine/ICollidable.h"
#include "engine/IOnDebug.h"
#include <SDL.h>
#include <algorithm>
#include <iostream>

namespace Engine {

Engine::~Engine() {
    shutdown();
}

bool Engine::init(const EngineConfig& config, RendererPtr renderer) {
    // Configure logger
    auto& logger = Logger::getInstance();
    logger.setMinLevel(config.logLevel);
    logger.setShowTimestamp(config.showTimestamp);
    logger.setShowWallClock(config.showWallClock);
    logger.setShowFrame(config.showFrame);
    logger.setShowLevel(config.showLogLevel);

    if (config.logToFile) {
        logger.setOutput(std::make_unique<FileOutput>(config.logFilePath));
        LOG_INFO("Logging to file: " + config.logFilePath);
    }

    LOG_INFO("Initializing engine...");

    // Create default SDL renderer if none provided
    if (!renderer) {
        renderer = std::make_unique<SDLRenderer>();
        LOG_DEBUG("Created default SDL renderer");
    }

    renderer_ = std::move(renderer);

    if (!renderer_->init(config.title, config.width, config.height)) {
        LOG_ERROR("Failed to initialize renderer");
        return false;
    }

    LOG_INFO_STREAM("Renderer initialized: " << config.width << "x" << config.height);

    // Initialize resource manager with renderer
    resourceManager_.init(renderer_.get());
    LOG_DEBUG("Resource manager initialized");

    running_ = true;
    frameNumber_ = 0;

    LOG_INFO("Engine initialization complete");
    return true;
}

// Backward compatible convenience method
bool Engine::init(const std::string& title, int width, int height, RendererPtr renderer) {
    EngineConfig config;
    config.title = title;
    config.width = width;
    config.height = height;
    return init(config, std::move(renderer));
}

void Engine::shutdown() {
    // Destroy all game objects
    for (auto& obj : gameObjects_) {
        obj->onDestroy();
    }
    gameObjects_.clear();
    updateables_.clear();
    fixedUpdateables_.clear();
    inputHandlers_.clear();
    renderables_.clear();
    collidables_.clear();
    debugObjects_.clear();

    clearLayers();

    // Shutdown resource manager before renderer
    resourceManager_.shutdown();

    if (renderer_) {
        renderer_->shutdown();
    }
    running_ = false;
}

std::shared_ptr<Layer> Engine::createLayer(int renderOrder) {
    auto layer = std::make_shared<Layer>(renderOrder);
    addLayer(layer);
    return layer;
}

void Engine::addLayer(const std::shared_ptr<Layer>& layer) {
    layers_.push_back(layer);
    sortLayers();
}

void Engine::removeLayer(const std::shared_ptr<Layer>& layer) {
    auto it = std::find(layers_.begin(), layers_.end(), layer);
    if (it != layers_.end()) {
        layers_.erase(it);
    }
}

void Engine::clearLayers() {
    layers_.clear();
}

void Engine::sortLayers() {
    std::sort(layers_.begin(), layers_.end(),
        [](const std::shared_ptr<Layer>& a, const std::shared_ptr<Layer>& b) {
            return a->getRenderOrder() < b->getRenderOrder();
        });
}

void Engine::render() {
    if (!renderer_) return;

    renderer_->clear();

    // Render all layers in order (lowest render order first)
    for (const auto& layer : layers_) {
        layer->render(*renderer_);
    }

    // Render IRenderable objects (sorted by render order)
    auto sortedRenderables = renderables_;
    std::sort(sortedRenderables.begin(), sortedRenderables.end(),
        [](const auto& a, const auto& b) {
            return a.second->getRenderOrder() < b.second->getRenderOrder();
        });

    for (const auto& [obj, renderable] : sortedRenderables) {
        if (obj->isActive()) {
            renderable->render(*renderer_);
        }
    }

    // Debug rendering
    if (debugMode_) {
        for (const auto& [obj, debugObj] : debugObjects_) {
            if (obj->isActive()) {
                debugObj->onDebug(*renderer_);
            }
        }
    }

    renderer_->present();
}

// GameObject management
void Engine::attachGameObject(GameObjectPtr object) {
    object->setEngine(this);
    gameObjects_.push_back(object);
    cacheInterfacePointers(object);
    object->onAttached();
}

void Engine::removeGameObject(GameObjectPtr object) {
    object->destroy();
}

void Engine::removeGameObject(const std::string& name) {
    for (auto& obj : gameObjects_) {
        if (obj->getName() == name) {
            obj->destroy();
            return;
        }
    }
}

GameObjectPtr Engine::findGameObject(const std::string& name) {
    for (auto& obj : gameObjects_) {
        if (obj->getName() == name) {
            return obj;
        }
    }
    return nullptr;
}

// Event polling
bool Engine::pollEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return false;
        }

        // Process event through input system
        input_.processEvent(e);
    }
    return true;
}

// Main update loop
void Engine::update(float deltaTime) {
    // Increment frame counter and update logger
    frameNumber_++;
    Logger::getInstance().setFrameNumber(frameNumber_);

    // Auto-calculate deltaTime if not provided
    if (deltaTime < 0.0f) {
        Uint32 currentTime = SDL_GetTicks();
        if (lastFrameTime_ == 0) {
            // First frame
            lastFrameTime_ = currentTime;
            deltaTime = 0.016f;  // Assume ~60 FPS for first frame
        } else {
            deltaTime = (currentTime - lastFrameTime_) / 1000.0f;
            lastFrameTime_ = currentTime;
        }

        // Cap delta time to prevent spiral of death
        if (deltaTime > 0.1f) deltaTime = 0.1f;
    }

    // Fixed update with accumulator
    fixedAccumulator_ += deltaTime;
    while (fixedAccumulator_ >= fixedTimestep_) {
        fixedUpdate();
        fixedAccumulator_ -= fixedTimestep_;
    }

    // Process input for this frame
    for (const auto& [obj, handler] : inputHandlers_) {
        if (obj->isActive()) {
            handler->handleInput(input_);
        }
    }

    // Variable timestep updates
    updateGameObjects(deltaTime);

    // Clean up objects marked for deletion
    cleanupDestroyedObjects();

    // End of frame: clear input state
    input_.update();
}

void Engine::processInput(SDL_Event& event) {
    input_.processEvent(event);
}

void Engine::updateGameObjects(float deltaTime) {
    for (const auto& [obj, updateable] : updateables_) {
        if (obj->isActive()) {
            updateable->update(deltaTime);
        }
    }
}

void Engine::fixedUpdate() {
    for (const auto& [obj, fixedUpdateable] : fixedUpdateables_) {
        if (obj->isActive()) {
            fixedUpdateable->fixedUpdate(fixedTimestep_);
        }
    }
}

void Engine::checkCollisions() {
    // Simple O(n²) collision check - can be optimized with spatial partitioning later
    for (size_t i = 0; i < collidables_.size(); ++i) {
        auto& [objA, collidableA] = collidables_[i];
        if (!objA->isActive()) continue;

        for (size_t j = i + 1; j < collidables_.size(); ++j) {
            auto& [objB, collidableB] = collidables_[j];
            if (!objB->isActive()) continue;

            // Check layer masks
            int layerA = collidableA->getCollisionLayer();
            int maskA = collidableA->getCollisionMask();
            int layerB = collidableB->getCollisionLayer();
            int maskB = collidableB->getCollisionMask();

            if (!(maskA & (1 << layerB)) || !(maskB & (1 << layerA))) {
                continue;
            }

            // Check AABB collision
            Rect boundsA = collidableA->getBounds();
            Rect boundsB = collidableB->getBounds();

            bool collision = (boundsA.x < boundsB.x + boundsB.w &&
                            boundsA.x + boundsA.w > boundsB.x &&
                            boundsA.y < boundsB.y + boundsB.h &&
                            boundsA.y + boundsA.h > boundsB.y);

            if (collision) {
                collidableA->onCollision(objB.get());
                collidableB->onCollision(objA.get());
            }
        }
    }
}

void Engine::cleanupDestroyedObjects() {
    // Remove from gameObjects list
    auto it = std::remove_if(gameObjects_.begin(), gameObjects_.end(),
        [](const GameObjectPtr& obj) {
            if (obj->isMarkedForDeletion()) {
                obj->onDestroy();
                return true;
            }
            return false;
        });

    if (it != gameObjects_.end()) {
        gameObjects_.erase(it, gameObjects_.end());

        // Rebuild interface caches
        updateables_.clear();
        fixedUpdateables_.clear();
        inputHandlers_.clear();
        renderables_.clear();
        collidables_.clear();
        debugObjects_.clear();

        for (auto& obj : gameObjects_) {
            cacheInterfacePointers(obj);
        }
    }
}

void Engine::cacheInterfacePointers(GameObjectPtr object) {
    // Cache interface pointers for fast access during updates
    if (auto* updateable = dynamic_cast<IUpdateable*>(object.get())) {
        updateables_.push_back({object, updateable});
    }
    if (auto* fixedUpdateable = dynamic_cast<IFixedUpdateable*>(object.get())) {
        fixedUpdateables_.push_back({object, fixedUpdateable});
    }
    if (auto* inputHandler = dynamic_cast<IInputHandler*>(object.get())) {
        inputHandlers_.push_back({object, inputHandler});
    }
    if (auto* renderable = dynamic_cast<IRenderable*>(object.get())) {
        renderables_.push_back({object, renderable});
    }
    if (auto* collidable = dynamic_cast<ICollidable*>(object.get())) {
        collidables_.push_back({object, collidable});
    }
    if (auto* debugObj = dynamic_cast<IOnDebug*>(object.get())) {
        debugObjects_.push_back({object, debugObj});
    }
}

} // namespace Engine
