# Resource Management System Design

## Problem Statement

Currently, resources (textures, fonts) are created ad-hoc, leading to:
- ❌ Duplicate loading of the same resource
- ❌ Manual lifetime management
- ❌ No centralized control over resources
- ❌ Destruction order issues (fonts vs TTF_Quit)
- ❌ Difficult to track what's loaded

## Goals

1. **Automatic Caching** - Load once, reuse everywhere
2. **Shared Ownership** - Reference counting via `shared_ptr`
3. **Named Resources** - Access by symbolic name
4. **Type Safety** - Compile-time type checking
5. **Lazy Loading** - Load on first request
6. **Proper Cleanup** - Automatic resource deallocation
7. **Extensible** - Easy to add new resource types

## Proposed Architecture

### Core Components

#### 1. Font Class (NEW)
Separate font loading from text rendering:

```cpp
class Font {
public:
    Font() = default;
    ~Font();

    bool loadFromFile(const std::string& path, int size);

    TTF_Font* getTTFFont() const { return font_; }
    bool isValid() const { return font_ != nullptr; }

    int getSize() const { return size_; }

private:
    TTF_Font* font_ = nullptr;
    int size_ = 0;
};

using FontPtr = std::shared_ptr<Font>;
```

#### 2. Updated Text Class
Uses Font instead of loading directly:

```cpp
class Text {
public:
    // Now takes a Font instead of loading itself
    void setFont(FontPtr font);
    void setText(const std::string& text, IRenderer& renderer, const Color& color);

    // ... rest of API
private:
    FontPtr font_;  // Shared ownership
    SDL_Texture* texture_ = nullptr;
};
```

#### 3. ResourceManager Class

```cpp
class ResourceManager {
public:
    ResourceManager() = default;
    ~ResourceManager() = default;

    // Must be initialized with renderer reference
    void init(IRenderer* renderer);
    void shutdown();

    // Texture management
    TexturePtr loadTexture(const std::string& name, const std::string& path);
    TexturePtr getTexture(const std::string& name);
    bool hasTexture(const std::string& name) const;
    void unloadTexture(const std::string& name);

    // Font management
    FontPtr loadFont(const std::string& name, const std::string& path, int size);
    FontPtr getFont(const std::string& name);
    bool hasFont(const std::string& name) const;
    void unloadFont(const std::string& name);

    // Bulk operations
    void clearTextures();
    void clearFonts();
    void clearAll();

    // Statistics
    size_t getTextureCount() const { return textures_.size(); }
    size_t getFontCount() const { return fonts_.size(); }

private:
    IRenderer* renderer_ = nullptr;
    std::unordered_map<std::string, TexturePtr> textures_;
    std::unordered_map<std::string, FontPtr> fonts_;
};
```

### Engine Integration

```cpp
class Engine {
public:
    // ... existing methods

    ResourceManager& getResourceManager() { return resourceManager_; }
    const ResourceManager& getResourceManager() const { return resourceManager_; }

private:
    ResourceManager resourceManager_;
};
```

## Usage Examples

### Example 1: Loading Resources

```cpp
// In main or during initialization
Engine engine;
engine.init("Game", 800, 600);

auto& rm = engine.getResourceManager();

// Load textures (automatically cached)
rm.loadTexture("player", "assets/player.png");
rm.loadTexture("enemy", "assets/enemy.png");
rm.loadTexture("bg", "assets/background.png");

// Load fonts (size is part of the key)
rm.loadFont("arial-24", "fonts/arial.ttf", 24);
rm.loadFont("arial-48", "fonts/arial.ttf", 48);
```

### Example 2: Using Resources in GameObjects

```cpp
class Player : public GameObject, public IUpdateable {
public:
    void onAttached() override {
        // Get resources from manager
        auto& rm = getEngine()->getResourceManager();
        auto texture = rm.getTexture("player");

        // Create sprite with shared texture
        sprite_ = std::make_shared<Sprite>(texture);
        sprite_->setPosition({100, 100});

        // Add to a layer for rendering
        auto layer = getEngine()->getLayers()[0];
        layer->addSprite(sprite_);
    }

    void onDestroy() override {
        // Clean up sprite from layer
        // Texture is automatically cleaned up via shared_ptr
    }

private:
    std::shared_ptr<Sprite> sprite_;
};
```

### Example 3: Using Fonts

```cpp
class HUD : public GameObject, public IUpdateable {
public:
    void onAttached() override {
        auto& rm = getEngine()->getResourceManager();
        font_ = rm.getFont("arial-24");

        scoreText_.setFont(font_);
        scoreText_.setText("Score: 0", getEngine()->getRenderer(), Color{255, 255, 255});
    }

    void update(float deltaTime) override {
        // Update score text as needed
        std::string text = "Score: " + std::to_string(score_);
        scoreText_.setText(text, getEngine()->getRenderer(), Color{255, 255, 255});
    }

private:
    FontPtr font_;
    Text scoreText_;
    int score_ = 0;
};
```

### Example 4: Lazy Loading

```cpp
// Load on first use - ResourceManager handles caching
class Enemy : public GameObject {
public:
    void onAttached() override {
        auto& rm = getEngine()->getResourceManager();

        // If already loaded, returns cached version
        // If not loaded, loads it now
        auto texture = rm.loadTexture("enemy", "assets/enemy.png");

        sprite_ = std::make_shared<Sprite>(texture);
    }

private:
    std::shared_ptr<Sprite> sprite_;
};
```

## Implementation Details

### Resource Naming Convention

```
Format: "<category>_<name>_<variant>"

Examples:
- "player_idle"
- "player_walk"
- "enemy_goblin"
- "ui_button_normal"
- "font_arial_24"
- "font_arial_48"
```

### Caching Strategy

1. **Load Method**:
   - Check if resource exists in cache
   - If yes: return cached shared_ptr (increases refcount)
   - If no: load from disk, add to cache, return new shared_ptr

2. **Get Method**:
   - Check if resource exists in cache
   - If yes: return cached shared_ptr
   - If no: return nullptr (or throw? TBD)

3. **Automatic Cleanup**:
   - When all shared_ptrs are destroyed, resource is freed
   - Cache maintains weak_ptr? Or strong shared_ptr?
   - **Decision**: Cache maintains strong shared_ptr until explicit unload

### Resource Lifecycle

```
┌─────────────────────────────────────┐
│ 1. loadTexture("player", "...")    │
│    - Check cache: not found         │
│    - Load from disk                 │
│    - Create shared_ptr              │
│    - Store in cache                 │
│    - Return shared_ptr (refcount=2) │
└─────────────────────────────────────┘
              ↓
┌─────────────────────────────────────┐
│ 2. getTexture("player")             │
│    - Check cache: found!            │
│    - Return cached shared_ptr       │
│    - Refcount increases             │
└─────────────────────────────────────┘
              ↓
┌─────────────────────────────────────┐
│ 3. All users release references     │
│    - Refcount decreases             │
│    - Cache still holds reference    │
│    - Resource stays in memory       │
└─────────────────────────────────────┘
              ↓
┌─────────────────────────────────────┐
│ 4. unloadTexture("player") OR       │
│    clearTextures() OR shutdown()    │
│    - Remove from cache              │
│    - If refcount=1, resource freed  │
│    - If refcount>1, freed later     │
└─────────────────────────────────────┘
```

## Design Decisions (FINAL)

### 1. API Style ✅ DECIDED
**Typed methods over templates**
- Use `loadTexture()`, `getTexture()`, `loadFont()`, `getFont()`
- NO template-based `load<T>()` - avoid template ugliness in early API
- Clear, explicit, type-safe

### 2. Font/Text Separation ✅ DECIDED
**Font and Text are separate classes**
- Font = TTF data (loaded once, shared)
- Text = Rendered string (many instances per font)
- **Philosophy**: Data is separate from usage (like Texture vs Sprite)
- Fixes destruction order issues

### 3. Caching Strategy ✅ DECIDED
**Strong references in cache**
- Cache stores `shared_ptr`, not `weak_ptr`
- Resources stay loaded until explicit unload
- Predictable performance, no surprise reloads
- Manual memory management via `unload()` or `clear()`

### 4. Error Handling ✅ DECIDED
**Return nullptr, no exceptions**
- C++ exceptions are complex and problematic
- `loadTexture()` returns `nullptr` on failure
- `getTexture()` returns `nullptr` if not in cache
- Caller must check: `if (!texture) { /* handle error */ }`

### 5. Engine Integration ✅ DECIDED
**Single master ResourceManager via engine**
- Access via `engine.getResourceManager()`
- One centralized resource manager per engine instance
- Easy access from anywhere with engine reference

### 6. Base Path ✅ DECIDED
**ResourceManager has base path for relative loads**
- Set on construction: `ResourceManager(basePath)`
- All load paths are relative to base path
- Example: base="/assets", load "player.png" → "/assets/player.png"

### 7. Loading Strategy ✅ DECIDED
**Immediate loading by default, NO lazy loading**
- `loadTexture()` loads immediately from disk
- Resources loaded upfront (level start, game start)
- Keep loaded and reuse via cache
- No lazy loading complexity in v1

### ✅ Strong References in Cache

**Cache stores `shared_ptr`, not `weak_ptr`:**

**Pros:**
- Resources stay loaded once loaded
- Predictable performance (no reload mid-game)
- Simple API

**Cons:**
- Resources not auto-freed when unused
- Need explicit unload for memory management

**Alternative (weak_ptr):**
- Resources auto-freed when no longer used
- More memory efficient
- But: unpredictable reloads, cache misses

**Decision:** Use **strong references** for predictability. Games typically load resources upfront or per-level, then keep them.

### ✅ Separate Font and Text

**Font**: The TTF file, loaded once, shared
**Text**: Rendered string, many instances per font

```cpp
auto font = rm.loadFont("arial-24", "arial.ttf", 24);

Text text1, text2, text3;
text1.setFont(font);  // Share font
text2.setFont(font);  // Share font
text3.setFont(font);  // Share font

text1.setText("Hello", renderer, Color{255,255,255});
text2.setText("World", renderer, Color{255,255,255});
```

### ✅ Renderer Dependency

ResourceManager needs IRenderer* to load textures. Options:

1. **Pass to each load call** - verbose, error-prone
2. **Store in ResourceManager** - simple, clean
3. **Static/global renderer** - bad practice

**Decision:** Store IRenderer* in ResourceManager, set during init

### ✅ Error Handling

When resource not found:

**Option A:** Return nullptr
```cpp
auto tex = rm.getTexture("missing");
if (!tex) { /* handle error */ }
```

**Option B:** Throw exception
```cpp
try {
    auto tex = rm.getTexture("missing");
} catch (ResourceNotFoundException& e) { }
```

**Option C:** Return placeholder
```cpp
auto tex = rm.getTexture("missing");  // Returns checkerboard placeholder
```

**Decision:**
- `load()` returns nullptr on failure (check with `if (!tex)`)
- `get()` returns nullptr if not in cache
- Can add `getOrLoad()` convenience method later

## Future Extensions

### Phase 1 (Now)
- ✅ Texture management
- ✅ Font management

### Phase 2 (Later)
- Audio resources (Sound, Music)
- Shader programs
- Tileset atlases
- Animation data

### Phase 3 (Advanced)
- Async loading
- Resource streaming
- Memory budgets
- Hot reloading
- Resource packs/bundles

## API Summary

```cpp
// Initialize (called by Engine::init)
resourceManager.init(&renderer);

// Load resources (caching)
auto tex = resourceManager.loadTexture("player", "assets/player.png");
auto font = resourceManager.loadFont("arial-24", "fonts/arial.ttf", 24);

// Get from cache
auto tex = resourceManager.getTexture("player");
auto font = resourceManager.getFont("arial-24");

// Check existence
if (resourceManager.hasTexture("player")) { }

// Unload
resourceManager.unloadTexture("player");
resourceManager.clearTextures();
resourceManager.clearAll();

// Stats
size_t count = resourceManager.getTextureCount();
```

## Implementation Plan

1. ✅ Design document (this)
2. Create Font class (separate from Text)
3. Update Text class to use FontPtr
4. Create ResourceManager class
5. Integrate into Engine
6. Update examples to use ResourceManager
7. Test and fix destruction order issues
