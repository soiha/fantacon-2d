#pragma once

#include <string>
#include <memory>

namespace Engine {

class Engine;

// Base class for all game objects
// GameObjects can implement various interfaces (IUpdateable, IInputHandler, etc.)
// to opt-in to engine features
class GameObject {
public:
    explicit GameObject(const std::string& name = "GameObject");
    virtual ~GameObject() = default;

    // Lifecycle callbacks (called by engine)
    virtual void onAttached() {}    // Called when attached to engine
    virtual void onDestroy() {}     // Called before removal from engine

    // Active state
    void setActive(bool active) { active_ = active; }
    bool isActive() const { return active_; }

    // Name (for debugging and finding objects)
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    // Engine reference (set automatically when attached)
    Engine* getEngine() const { return engine_; }

    // Internal (called by engine)
    void setEngine(Engine* engine) { engine_ = engine; }

    // Mark for deletion at end of frame
    void destroy() { markedForDeletion_ = true; }
    bool isMarkedForDeletion() const { return markedForDeletion_; }

protected:
    std::string name_;
    bool active_ = true;
    bool markedForDeletion_ = false;
    Engine* engine_ = nullptr;
};

using GameObjectPtr = std::shared_ptr<GameObject>;

} // namespace Engine
