#include "engine/SDLRenderer.h"
#include "engine/Sprite.h"
#include "engine/Tilemap.h"
#include "engine/Text.h"
#include "engine/PixelBuffer.h"
#include "engine/IndexedPixelBuffer.h"
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>
#include <cmath>

namespace Engine {

SDLRenderer::~SDLRenderer() {
    shutdown();
}

bool SDLRenderer::init(const std::string& title, int width, int height) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "SDL_image could not initialize! IMG_Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    // Create window
    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN
    );

    if (!window_) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    // Create renderer
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window_);
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    // Set blend mode for transparency
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    // Store viewport dimensions
    windowWidth_ = width;
    windowHeight_ = height;

    return true;
}

void SDLRenderer::shutdown() {
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

void SDLRenderer::clear() {
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
}

void SDLRenderer::present() {
    SDL_RenderPresent(renderer_);
}

void SDLRenderer::renderSprite(const Sprite& sprite, const Vec2& layerOffset, float opacity) {
    if (!sprite.isVisible() || !sprite.getTexture() || !sprite.getTexture()->isValid()) {
        return;
    }

    auto texture = sprite.getTexture();
    Vec2 pos = sprite.getLayerPosition() + layerOffset;  // Apply layer offset
    const auto& scale = sprite.getScale();
    float rotation = sprite.getRotation();

    // Determine source rectangle
    SDL_Rect srcRect;
    if (sprite.hasSourceRect()) {
        const auto& src = sprite.getSourceRect();
        srcRect = {
            static_cast<int>(src.x),
            static_cast<int>(src.y),
            static_cast<int>(src.w),
            static_cast<int>(src.h)
        };
    } else {
        srcRect = {0, 0, texture->getWidth(), texture->getHeight()};
    }

    // Calculate destination rectangle
    int dstWidth = static_cast<int>(srcRect.w * scale.x);
    int dstHeight = static_cast<int>(srcRect.h * scale.y);

    SDL_Rect dstRect = {
        static_cast<int>(pos.x),
        static_cast<int>(pos.y),
        dstWidth,
        dstHeight
    };

    // Render with rotation
    SDL_Point center = {dstWidth / 2, dstHeight / 2};
    SDL_RenderCopyEx(
        renderer_,
        texture->getSDLTexture(),
        &srcRect,
        &dstRect,
        rotation,
        &center,
        SDL_FLIP_NONE
    );
}

void SDLRenderer::renderTilemap(const Tilemap& tilemap, const Vec2& layerOffset, float opacity) {
    if (!tilemap.isVisible() || !tilemap.getTileset() || !tilemap.getTileset()->isValid()) {
        return;
    }

    auto tileset = tilemap.getTileset();
    Vec2 pos = tilemap.getPosition() + layerOffset;  // Apply layer offset
    int tileWidth = tilemap.getTileWidth();
    int tileHeight = tilemap.getTileHeight();
    int tilesPerRow = tilemap.getTilesPerRow();

    if (tilesPerRow <= 0) return;

    for (int y = 0; y < tilemap.getHeight(); ++y) {
        for (int x = 0; x < tilemap.getWidth(); ++x) {
            int tileId = tilemap.getTile(x, y);
            if (tileId < 0) continue;  // Skip empty tiles

            // Calculate source rectangle in tileset
            int srcX = (tileId % tilesPerRow) * tileWidth;
            int srcY = (tileId / tilesPerRow) * tileHeight;
            SDL_Rect srcRect = {srcX, srcY, tileWidth, tileHeight};

            // Calculate destination rectangle
            SDL_Rect dstRect = {
                static_cast<int>(pos.x + x * tileWidth),
                static_cast<int>(pos.y + y * tileHeight),
                tileWidth,
                tileHeight
            };

            SDL_RenderCopy(renderer_, tileset->getSDLTexture(), &srcRect, &dstRect);
        }
    }
}

void SDLRenderer::renderText(const Text& text, const Vec2& position, float opacity) {
    if (!text.isValid()) {
        return;
    }

    SDL_Rect dstRect = {
        static_cast<int>(position.x),
        static_cast<int>(position.y),
        text.getWidth(),
        text.getHeight()
    };

    SDL_RenderCopy(renderer_, text.getTexture(), nullptr, &dstRect);
}

void SDLRenderer::renderPixelBuffer(const PixelBuffer& buffer, const Vec2& layerOffset, float opacity) {
    if (!buffer.isVisible() || !buffer.getTexture()) {
        return;
    }

    // Upload pixels if dirty
    const_cast<PixelBuffer&>(buffer).upload(*this);

    // Calculate position with layer offset
    Vec2 pos = buffer.getPosition() + layerOffset;

    // Calculate destination rectangle with scale
    float scale = buffer.getScale();
    SDL_Rect dstRect = {
        static_cast<int>(pos.x),
        static_cast<int>(pos.y),
        static_cast<int>(buffer.getWidth() * scale),
        static_cast<int>(buffer.getHeight() * scale)
    };

    // Render the pixel buffer texture
    SDL_RenderCopy(renderer_, static_cast<SDL_Texture*>(buffer.getTexture()->getHandle()), nullptr, &dstRect);
}

TexturePtr SDLRenderer::createStreamingTexture(int width, int height) {
    // Create a streaming texture (can be updated with pixel data)
    SDL_Texture* sdlTexture = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );

    if (!sdlTexture) {
        return nullptr;
    }

    // Wrap in our Texture class
    auto texture = std::make_shared<Texture>();
    texture->setHandle(sdlTexture);
    texture->setDimensions(width, height);

    return texture;
}

void SDLRenderer::updateTexture(Texture& texture, const Color* pixels, int width, int height) {
    SDL_Texture* sdlTexture = static_cast<SDL_Texture*>(texture.getHandle());
    if (!sdlTexture) {
        return;
    }

    // Lock texture for writing
    void* texturePixels;
    int pitch;
    if (SDL_LockTexture(sdlTexture, nullptr, &texturePixels, &pitch) != 0) {
        return;
    }

    // Copy pixel data (convert from Color to RGBA32)
    Uint32* dest = static_cast<Uint32*>(texturePixels);
    for (int i = 0; i < width * height; ++i) {
        const Color& c = pixels[i];
        // RGBA32 format: 0xRRGGBBAA
        dest[i] = (c.r << 24) | (c.g << 16) | (c.b << 8) | c.a;
    }

    // Unlock texture
    SDL_UnlockTexture(sdlTexture);
}

void SDLRenderer::renderIndexedPixelBuffer(const IndexedPixelBuffer& buffer, const Vec2& layerOffset, float opacity) {
    if (!buffer.isVisible() || !buffer.getTexture()) {
        return;
    }

    // Upload pixels if dirty (converts indexed colors to RGBA using palette)
    const_cast<IndexedPixelBuffer&>(buffer).upload(*this);

    // Calculate position with layer offset
    Vec2 pos = buffer.getPosition() + layerOffset;

    // Calculate destination rectangle with scale
    float scale = buffer.getScale();
    SDL_Rect dstRect = {
        static_cast<int>(pos.x),
        static_cast<int>(pos.y),
        static_cast<int>(buffer.getWidth() * scale),
        static_cast<int>(buffer.getHeight() * scale)
    };

    // Render the indexed pixel buffer texture
    SDL_RenderCopy(renderer_, static_cast<SDL_Texture*>(buffer.getTexture()->getHandle()), nullptr, &dstRect);
}

} // namespace Engine
