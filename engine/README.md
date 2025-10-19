# 2D Game Engine

A lightweight, modular 2D game engine built with C++ and SDL2. Features a flexible layer-based rendering system with support for sprites and tilemaps.

## Features

- **Sprite Management**: Render arbitrary numbers of sprites with position, rotation, and scale
- **Tilemap Support**: Efficient tile-based rendering for backgrounds and levels
- **Layer System**: Multiple rendering layers with configurable render order
- **Modular Renderer**: Abstract renderer interface allows easy swapping of backends (SDL, Vulkan, headless, etc.)
- **RAII Resource Management**: Safe and automatic resource cleanup
- **Modern C++17**: Uses modern C++ features and best practices

## Architecture

### Core Components

- **IRenderer**: Abstract renderer interface for backend independence
- **SDLRenderer**: SDL2-based renderer implementation
- **Sprite**: Renderable object with transform properties (position, rotation, scale)
- **Tilemap**: Grid-based tile rendering system
- **Layer**: Container for sprites and tilemaps with render order
- **Engine**: Main engine class that manages layers and rendering
- **Texture**: RAII wrapper for texture resources

### Rendering Pipeline

1. Layers are sorted by render order (lowest to highest)
2. For each layer:
   - Render all tilemaps (typically backgrounds)
   - Render all sprites (typically game objects)
3. Layers render on top of each other in order

## Building

### Prerequisites

- C++17 compatible compiler
- SDL2
- SDL2_image
- CMake 3.10+ (for CMake build) or Make (for Makefile build)

The project supports **two build systems**: CMake (recommended, cross-platform) and Make (simple, Unix-like systems).

### macOS

#### Option 1: CMake (Recommended)

```bash
# Install dependencies
brew install sdl2 sdl2_image cmake

# Build
mkdir build && cd build
cmake ..
make

# Run example
./example
```

#### Option 2: Makefile

```bash
# Install dependencies
brew install sdl2 sdl2_image

# Build
make

# Run example
./build/example
```

### Linux

#### Option 1: CMake (Recommended)

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install libsdl2-dev libsdl2-image-dev cmake

# Build
mkdir build && cd build
cmake ..
make

# Run example
./example
```

#### Option 2: Makefile

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install libsdl2-dev libsdl2-image-dev

# Build
make

# Run example
./build/example
```

### Windows

**Note**: CMake is recommended for Windows.

```bash
# Install dependencies using vcpkg
vcpkg install sdl2 sdl2-image

# Build with CMake
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
cmake --build .

# Run example
.\Debug\example.exe
```

### Cleaning Build Artifacts

```bash
# For CMake builds
rm -rf build

# For Makefile builds
make clean
```

### Troubleshooting

#### CMake can't find SDL2_image

If you get an error like:
```
CMake Error: By not providing "FindSDL2_image.cmake"...
```

**Solution**: The CMakeLists.txt now uses `pkg-config` to find SDL2 and SDL2_image, which is more reliable. Make sure you have:
1. SDL2 and SDL2_image installed
2. pkg-config installed (`brew install pkg-config` on macOS)
3. A clean build directory (`rm -rf build`)

#### Headers not found (SDL.h)

If you get `#include <SDL.h> file not found`:
- The project uses `#include <SDL.h>` (not `#include <SDL2/SDL.h>`)
- CMake and Make both configure the include paths correctly
- For custom builds, use `sdl2-config --cflags` to get the correct flags

## Usage

### Basic Example

```cpp
#include "engine/Engine.h"
#include "engine/Sprite.h"
#include "engine/Tilemap.h"

using namespace Engine;

int main() {
    // Initialize engine
    Engine engine;
    engine.init("My Game", 800, 600);

    // Create layers
    auto backgroundLayer = engine.createLayer(0);
    auto gameLayer = engine.createLayer(10);
    auto uiLayer = engine.createLayer(20);

    // Create and load a texture
    auto texture = std::make_shared<Texture>();
    texture->loadFromFile("sprite.png", engine.getRenderer());

    // Create a sprite
    auto sprite = std::make_shared<Sprite>(texture, Vec2{100, 100});
    sprite->setScale(2.0f);
    sprite->setRotation(45.0f);
    gameLayer->addSprite(sprite);

    // Create a tilemap
    auto tilemap = std::make_shared<Tilemap>(20, 15, 32, 32);
    auto tileset = std::make_shared<Texture>();
    tileset->loadFromFile("tileset.png", engine.getRenderer());
    tilemap->setTileset(tileset, 8);  // 8 tiles per row

    // Set tiles
    for (int y = 0; y < tilemap->getHeight(); ++y) {
        for (int x = 0; x < tilemap->getWidth(); ++x) {
            tilemap->setTile(x, y, (x + y) % 16);
        }
    }
    backgroundLayer->addTilemap(tilemap);

    // Game loop
    SDL_Event e;
    while (engine.isRunning()) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) engine.quit();
        }

        // Update game logic here
        sprite->setRotation(sprite->getRotation() + 1.0f);

        // Render
        engine.render();
    }

    engine.shutdown();
    return 0;
}
```

### Sprite API

```cpp
// Create sprite
auto sprite = std::make_shared<Sprite>();

// Transform
sprite->setPosition({100, 200});
sprite->setRotation(45.0f);  // degrees
sprite->setScale({2.0f, 2.0f});  // or setScale(2.0f) for uniform

// Texture
sprite->setTexture(myTexture);

// Source rectangle (for sprite sheets)
sprite->setSourceRect({0, 0, 32, 32});

// Visibility
sprite->setVisible(false);
```

### Tilemap API

```cpp
// Create tilemap (width, height in tiles, tile dimensions in pixels)
auto tilemap = std::make_shared<Tilemap>(10, 10, 32, 32);

// Set tileset
tilemap->setTileset(tilesetTexture, 8);  // 8 tiles per row

// Set individual tiles
tilemap->setTile(0, 0, 5);  // Set tile at (0,0) to tile ID 5

// Fill with a tile
tilemap->fill(0);

// Position
tilemap->setPosition({100, 100});
```

### Layer API

```cpp
// Create layer with render order
auto layer = std::make_shared<Layer>(10);

// Add objects
layer->addSprite(sprite);
layer->addTilemap(tilemap);

// Remove objects
layer->removeSprite(sprite);
layer->removeTilemap(tilemap);

// Clear all
layer->clearSprites();
layer->clearTilemaps();

// Visibility
layer->setVisible(false);
```

### Custom Renderer Backend

The engine uses an abstract `IRenderer` interface, making it easy to swap rendering backends:

```cpp
// Create custom renderer
class VulkanRenderer : public IRenderer {
    // Implement interface methods...
};

// Use custom renderer
Engine engine;
auto customRenderer = std::make_unique<VulkanRenderer>();
engine.init("My Game", 800, 600, std::move(customRenderer));
```

## Project Structure

```
engine/
├── include/engine/     # Public headers
│   ├── Engine.h
│   ├── IRenderer.h
│   ├── SDLRenderer.h
│   ├── Renderer.h      # Alias for SDLRenderer
│   ├── Sprite.h
│   ├── Tilemap.h
│   ├── Layer.h
│   ├── Texture.h
│   └── Types.h
├── src/                # Implementation files
│   ├── Engine.cpp
│   ├── SDLRenderer.cpp
│   ├── Sprite.cpp
│   ├── Tilemap.cpp
│   ├── Layer.cpp
│   └── Texture.cpp
├── examples/           # Example programs
│   └── main.cpp
├── assets/             # Game assets (textures, etc.)
├── CMakeLists.txt
└── README.md
```

## Design Decisions

### Abstract Renderer Interface

The renderer uses an abstract interface (`IRenderer`) to allow easy swapping of rendering backends. This enables:
- Testing with headless renderers
- Migration to different graphics APIs (Vulkan, DirectX, etc.)
- Platform-specific optimizations

### Layer-Based Rendering

Objects are organized into layers with explicit render order:
- Predictable rendering order
- Easy z-ordering of game objects
- Efficient batch rendering within layers
- Simple visibility toggling per layer

### Shared Pointers for Game Objects

Sprites, tilemaps, and layers use `shared_ptr` for:
- Safe object lifetime management
- Easy sharing between systems
- Automatic cleanup
- Simple removal from containers

### RAII Resource Management

All engine resources use RAII principles:
- Textures automatically clean up SDL resources
- Renderer manages window and context lifecycle
- No manual cleanup required in most cases

## Future Enhancements

- [ ] Camera system with viewport and zoom
- [ ] Sprite animation support
- [ ] Particle system
- [ ] Text rendering
- [ ] Input management system
- [ ] Audio system
- [ ] Scene management
- [ ] Collision detection
- [ ] Asset manager with caching
- [ ] Shader support

## License

MIT License - feel free to use this in your projects!
