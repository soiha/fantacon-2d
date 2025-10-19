#pragma once

#include "IRenderer.h"
#include "Layer.h"
#include "GameObject.h"
#include "Input.h"
#include "ResourceManager.h"
#include "Logger.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <string>

namespace Engine {

class IUpdateable;
class IFixedUpdateable;
class IInputHandler;
class IRenderable;
class ICollidable;
class IOnDebug;

// Engine initialization options
struct EngineConfig {
    // Window settings
    std::string title = "Engine";
    int width = 800;
    int height = 600;

    // Logger settings
    LogLevel logLevel = LogLevel::Info;
    bool logToFile = false;
    std::string logFilePath = "engine.log";
    bool showTimestamp = true;
    bool showWallClock = true;
    bool showFrame = true;
    bool showLogLevel = true;
};

class Engine {
public:
    Engine() = default;
    ~Engine();

    // Initialize with specific renderer and config
    bool init(const EngineConfig& config, RendererPtr renderer = nullptr);

    // Convenience init (backward compatible)
    bool init(const std::string& title, int width, int height, RendererPtr renderer = nullptr);

    void shutdown();

    // Layer management
    std::shared_ptr<Layer> createLayer(int renderOrder = 0);
    void addLayer(const std::shared_ptr<Layer>& layer);
    void removeLayer(const std::shared_ptr<Layer>& layer);
    void clearLayers();
    void sortLayers();  // Sort layers by render order

    // GameObject management
    void attachGameObject(GameObjectPtr object);
    void removeGameObject(GameObjectPtr object);
    void removeGameObject(const std::string& name);
    GameObjectPtr findGameObject(const std::string& name);
    template<typename T>
    std::vector<std::shared_ptr<T>> findGameObjectsOfType();

    // Event polling (call once per frame, returns false if quit requested)
    bool pollEvents();

    // Main frame update (call this every frame)
    // If deltaTime < 0 (default), automatically calculates delta time from last frame
    void update(float deltaTime = -1.0f);

    // Rendering
    void render();

    // Input
    const Input& getInput() const { return input_; }

    // Resource management
    ResourceManager& getResourceManager() { return resourceManager_; }
    const ResourceManager& getResourceManager() const { return resourceManager_; }

    // Getters
    IRenderer& getRenderer() { return *renderer_; }
    const std::vector<std::shared_ptr<Layer>>& getLayers() const { return layers_; }
    const std::vector<GameObjectPtr>& getGameObjects() const { return gameObjects_; }

    bool isRunning() const { return running_; }
    void quit() { running_ = false; }

    // Fixed timestep configuration
    void setFixedTimestep(float timestep) { fixedTimestep_ = timestep; }
    float getFixedTimestep() const { return fixedTimestep_; }

    // Debug mode
    void setDebugMode(bool debug) { debugMode_ = debug; }
    bool isDebugMode() const { return debugMode_; }

    // Collision detection (call manually or automatically in update)
    void checkCollisions();

private:
    void processInput(SDL_Event& event);
    void updateGameObjects(float deltaTime);
    void fixedUpdate();
    void cleanupDestroyedObjects();
    void cacheInterfacePointers(GameObjectPtr object);

    RendererPtr renderer_;
    std::vector<std::shared_ptr<Layer>> layers_;
    std::vector<GameObjectPtr> gameObjects_;
    Input input_;
    ResourceManager resourceManager_;

    // Interface caches for performance
    std::vector<std::pair<GameObjectPtr, IUpdateable*>> updateables_;
    std::vector<std::pair<GameObjectPtr, IFixedUpdateable*>> fixedUpdateables_;
    std::vector<std::pair<GameObjectPtr, IInputHandler*>> inputHandlers_;
    std::vector<std::pair<GameObjectPtr, IRenderable*>> renderables_;
    std::vector<std::pair<GameObjectPtr, ICollidable*>> collidables_;
    std::vector<std::pair<GameObjectPtr, IOnDebug*>> debugObjects_;

    // Fixed timestep accumulator
    float fixedTimestep_ = 1.0f / 60.0f;  // 60Hz default
    float fixedAccumulator_ = 0.0f;

    // Automatic delta time tracking
    Uint32 lastFrameTime_ = 0;

    // Frame counter
    uint64_t frameNumber_ = 0;

    bool running_ = false;
    bool debugMode_ = false;
};

// Template implementation
template<typename T>
std::vector<std::shared_ptr<T>> Engine::findGameObjectsOfType() {
    std::vector<std::shared_ptr<T>> result;
    for (const auto& obj : gameObjects_) {
        auto casted = std::dynamic_pointer_cast<T>(obj);
        if (casted) {
            result.push_back(casted);
        }
    }
    return result;
}

} // namespace Engine
