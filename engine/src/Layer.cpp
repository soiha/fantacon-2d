#include "engine/Layer.h"
#include "engine/IRenderer.h"
#include <algorithm>

namespace Engine {

Layer::Layer(int renderOrder)
    : renderOrder_(renderOrder) {
}

void Layer::addSprite(const std::shared_ptr<Sprite>& sprite) {
    sprites_.push_back(sprite);
}

void Layer::removeSprite(const std::shared_ptr<Sprite>& sprite) {
    auto it = std::find(sprites_.begin(), sprites_.end(), sprite);
    if (it != sprites_.end()) {
        sprites_.erase(it);
    }
}

void Layer::clearSprites() {
    sprites_.clear();
}

void Layer::addTilemap(const std::shared_ptr<Tilemap>& tilemap) {
    tilemaps_.push_back(tilemap);
}

void Layer::removeTilemap(const std::shared_ptr<Tilemap>& tilemap) {
    auto it = std::find(tilemaps_.begin(), tilemaps_.end(), tilemap);
    if (it != tilemaps_.end()) {
        tilemaps_.erase(it);
    }
}

void Layer::clearTilemaps() {
    tilemaps_.clear();
}

void Layer::addPixelBuffer(const std::shared_ptr<PixelBuffer>& buffer) {
    pixelBuffers_.push_back(buffer);
}

void Layer::removePixelBuffer(const std::shared_ptr<PixelBuffer>& buffer) {
    auto it = std::find(pixelBuffers_.begin(), pixelBuffers_.end(), buffer);
    if (it != pixelBuffers_.end()) {
        pixelBuffers_.erase(it);
    }
}

void Layer::clearPixelBuffers() {
    pixelBuffers_.clear();
}

void Layer::addIndexedPixelBuffer(const std::shared_ptr<IndexedPixelBuffer>& buffer) {
    indexedPixelBuffers_.push_back(buffer);
}

void Layer::removeIndexedPixelBuffer(const std::shared_ptr<IndexedPixelBuffer>& buffer) {
    auto it = std::find(indexedPixelBuffers_.begin(), indexedPixelBuffers_.end(), buffer);
    if (it != indexedPixelBuffers_.end()) {
        indexedPixelBuffers_.erase(it);
    }
}

void Layer::clearIndexedPixelBuffers() {
    indexedPixelBuffers_.clear();
}

void Layer::render(IRenderer& renderer) {
    if (!visible_) return;

    // Render tilemaps first (usually background)
    for (const auto& tilemap : tilemaps_) {
        renderer.renderTilemap(*tilemap, offset_);
    }

    // Render pixel buffers (can be under or over sprites depending on order added)
    for (const auto& buffer : pixelBuffers_) {
        renderer.renderPixelBuffer(*buffer, offset_);
    }

    // Render indexed pixel buffers
    for (const auto& buffer : indexedPixelBuffers_) {
        renderer.renderIndexedPixelBuffer(*buffer, offset_);
    }

    // Render sprites on top
    for (const auto& sprite : sprites_) {
        renderer.renderSprite(*sprite, offset_);
    }
}

} // namespace Engine
