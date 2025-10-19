# GameObject System Design

## Overview

A flexible, interface-based GameObject system that allows game objects to opt-in to engine features (input handling, time updates, rendering, etc.) by implementing specific interfaces.

## Core Concepts

### 1. GameObject Base Class

```cpp
class GameObject {
public:
    GameObject(const std::string& name = "GameObject");
    virtual ~GameObject() = default;

    // Lifecycle (called by engine)
    virtual void onAttached() {}   // Called when attached to engine
    virtual void onDestroy() {}    // Called when removed from engine

    // State management
    void setActive(bool active);
    bool isActive() const;

    const std::string& getName() const;
    void setName(const std::string& name);

    // Note: GameObjects manage their own sprites if needed
    // They are responsible for creating and destroying them

private:
    std::string name_;
    bool active_ = true;
};
```

**Design Decision**: GameObjects own and manage their own sprites if they need them. They create them in their constructor or `onAttached()`, and are responsible for proper cleanup. The engine does NOT manage GameObject sprites automatically.

### 2. Interface-based System

Objects implement interfaces to receive specific callbacks:

#### IUpdateable - Time-aware Updates

```cpp
class IUpdateable {
public:
    virtual ~IUpdateable() = default;

    // Called every frame with variable delta time
    virtual void update(float deltaTime) = 0;
};

class IFixedUpdateable {
public:
    virtual ~IFixedUpdateable() = default;

    // Called at fixed intervals (e.g., 60Hz for physics)
    virtual void fixedUpdate(float fixedDeltaTime) = 0;
};
```

#### IInputHandler - Input Events

```cpp
class IInputHandler {
public:
    virtual ~IInputHandler() = default;

    // Called with input state each frame
    virtual void handleInput(const Input& input) = 0;
};
```

#### IRenderable - Custom Rendering

```cpp
class IRenderable {
public:
    virtual ~IRenderable() = default;

    // Custom rendering (in addition to sprite)
    virtual void render(IRenderer& renderer) = 0;

    // Render order (lower = rendered first)
    virtual int getRenderOrder() const { return 0; }
};
```

#### ICollidable - Collision Detection

```cpp
class ICollidable {
public:
    virtual ~ICollidable() = default;

    // Get collision bounds
    virtual Rect getBounds() const = 0;

    // Called when collision occurs
    virtual void onCollision(GameObject* other) = 0;

    // Collision layer/mask for filtering
    virtual int getCollisionLayer() const { return 0; }
    virtual int getCollisionMask() const { return 0xFFFFFFFF; }
};
```

#### IOnDebug - Debug Visualization

```cpp
class IOnDebug {
public:
    virtual ~IOnDebug() = default;

    // Called during debug rendering pass
    virtual void onDebug(IRenderer& renderer) = 0;
};
```

### 3. Input System

```cpp
class Input {
public:
    // Keyboard state
    bool isKeyPressed(SDL_Keycode key) const;   // Pressed this frame
    bool isKeyHeld(SDL_Keycode key) const;      // Held down
    bool isKeyReleased(SDL_Keycode key) const;  // Released this frame

    // Mouse state
    Vec2 getMousePosition() const;
    Vec2 getMouseDelta() const;
    bool isMouseButtonPressed(int button) const;
    bool isMouseButtonHeld(int button) const;
    bool isMouseButtonReleased(int button) const;

    // Internal (called by engine)
    void processEvent(const SDL_Event& event);
    void update();  // Called at end of frame to update state
};
```

### 4. Engine Integration

```cpp
class Engine {
public:
    // GameObject management
    void addGameObject(std::shared_ptr<GameObject> object);
    void removeGameObject(std::shared_ptr<GameObject> object);
    void removeGameObject(const std::string& name);

    // Find objects
    std::shared_ptr<GameObject> findGameObject(const std::string& name);
    template<typename T>
    std::vector<std::shared_ptr<T>> findGameObjectsOfType();

    // Input access
    const Input& getInput() const;

private:
    std::vector<std::shared_ptr<GameObject>> gameObjects_;
    Input input_;

    // Internal update loop
    void processInput();      // Poll events, update Input
    void updateGameObjects(float deltaTime);
    void renderGameObjects();
};
```

## Frame Loop

```
┌─────────────────────────────────────┐
│ 1. Process Input                    │
│    - Poll SDL events                │
│    - Update Input state             │
│    - Call IInputHandler::handleInput│
└─────────────────────────────────────┘
              ↓
┌─────────────────────────────────────┐
│ 2. Fixed Update (if needed)         │
│    - Call IFixedUpdateable::        │
│      fixedUpdate(fixedDelta)        │
└─────────────────────────────────────┘
              ↓
┌─────────────────────────────────────┐
│ 3. Update GameObjects               │
│    - Call IUpdateable::             │
│      update(deltaTime)              │
└─────────────────────────────────────┘
              ↓
┌─────────────────────────────────────┐
│ 4. Render                           │
│    - Clear screen                   │
│    - Render layers (sprites)        │
│    - Call IRenderable::render()     │
│    - Present                        │
└─────────────────────────────────────┘
```

## Example Usage

### Example 1: Simple Moving Object

```cpp
class MovingBox : public GameObject, public IUpdateable {
public:
    MovingBox() : GameObject("MovingBox") {
        auto sprite = std::make_shared<Sprite>();
        // ... set up sprite
        setSprite(sprite);
    }

    void update(float deltaTime) override {
        velocity_.y += gravity_ * deltaTime;

        auto pos = getSprite()->getPosition();
        pos.x += velocity_.x * deltaTime;
        pos.y += velocity_.y * deltaTime;
        getSprite()->setPosition(pos);
    }

private:
    Vec2 velocity_{100, 0};
    float gravity_ = 980.0f;
};

// Usage:
auto box = std::make_shared<MovingBox>();
engine.addGameObject(box);
```

### Example 2: Player with Input

```cpp
class Player : public GameObject,
               public IUpdateable,
               public IInputHandler {
public:
    Player() : GameObject("Player") {
        // Setup sprite...
    }

    void handleInput(const Input& input) override {
        velocity_.x = 0;

        if (input.isKeyHeld(SDLK_LEFT)) {
            velocity_.x = -speed_;
        }
        if (input.isKeyHeld(SDLK_RIGHT)) {
            velocity_.x = speed_;
        }
        if (input.isKeyPressed(SDLK_SPACE) && onGround_) {
            velocity_.y = -jumpForce_;
        }
    }

    void update(float deltaTime) override {
        // Apply gravity
        velocity_.y += gravity_ * deltaTime;

        // Update position
        auto pos = getSprite()->getPosition();
        pos.x += velocity_.x * deltaTime;
        pos.y += velocity_.y * deltaTime;
        getSprite()->setPosition(pos);
    }

private:
    Vec2 velocity_{0, 0};
    float speed_ = 200.0f;
    float jumpForce_ = 500.0f;
    float gravity_ = 980.0f;
    bool onGround_ = false;
};
```

### Example 3: Custom Rendering

```cpp
class DebugGrid : public GameObject, public IRenderable {
public:
    void render(IRenderer& renderer) override {
        // Custom debug visualization
        // Could use raw SDL calls or add debug drawing to renderer
    }

    int getRenderOrder() const override {
        return 1000;  // Render on top
    }
};
```

## Design Decisions

### ✅ Pros of Interface-based Design

1. **Explicit Opt-in**: Objects only implement what they need
2. **Type Safety**: Compile-time interface checking
3. **Performance**: No runtime type lookups, easy to cache by interface
4. **Clear Intent**: Looking at class declaration shows capabilities
5. **Simple**: Easy to understand and debug

### ⚠️ Considerations

1. **Multiple Inheritance**: C++ supports it, but can be confusing
   - **Mitigation**: All interfaces are pure virtual, no diamond problem

2. **Interface Discovery**: Engine needs to check `dynamic_cast`
   - **Mitigation**: Cache interface pointers on addGameObject()

3. **Sprite Management**: GameObject owns sprite, but Layer renders it
   - **Solution**: GameObject registers sprite with layer on creation

### 🔄 Alternative Considered: Component System

We considered an ECS-style component system but decided against it for this engine:
- **Complexity**: Adds significant complexity for small projects
- **Overhead**: Component lookup and management overhead
- **Learning Curve**: Harder for beginners
- **Flexibility**: Interface approach is "good enough" for most 2D games

## Design Decisions (FINAL)

1. **Sprite Management**: ✅ DECIDED
   - GameObjects own and manage their own sprites if needed
   - They create sprites themselves and handle cleanup
   - Engine does NOT automatically manage GameObject sprites

2. **Lifecycle Naming**: ✅ DECIDED
   - Use `onAttached()` when GameObject is attached to engine
   - Use `onDestroy()` when GameObject is removed
   - AVOID Unity/Unreal terminology (no "Start", "Awake", etc.)

3. **Fixed Timesteps**: ✅ DECIDED
   - Implement fixed timestep from the beginning
   - Use 60Hz (1/60s = 0.0166s) as default
   - Use accumulator pattern for stability

4. **Additional Interfaces**: ✅ DECIDED
   - Implement `ICollidable` for collision detection
   - Implement `IOnDebug` for debug visualization
   - DEFER serialization for future

5. **Object Removal**: ✅ DECIDED
   - Mark for deletion, clean up at end of frame
   - Safer during iteration, prevents iterator invalidation

## Implementation Plan

### Phase 1: Core GameObject System
1. GameObject base class
2. IUpdateable interface
3. Engine GameObject management
4. Sprite integration

### Phase 2: Input System
1. Input class with state tracking
2. IInputHandler interface
3. Engine input processing

### Phase 3: Advanced Features
1. IFixedUpdateable for physics
2. IRenderable for custom rendering
3. Helper methods and utilities

### Phase 4: Polish
1. GameObject lifecycle events
2. Find/query methods
3. Examples and documentation
