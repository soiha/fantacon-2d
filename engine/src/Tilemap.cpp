#include "engine/Tilemap.h"

namespace Engine {

Tilemap::Tilemap(int width, int height, int tileWidth, int tileHeight)
    : width_(width)
    , height_(height)
    , tileWidth_(tileWidth)
    , tileHeight_(tileHeight)
    , tiles_(width * height, -1) {
}

void Tilemap::setTile(int x, int y, int tileId) {
    if (x >= 0 && x < width_ && y >= 0 && y < height_) {
        tiles_[y * width_ + x] = tileId;
    }
}

int Tilemap::getTile(int x, int y) const {
    if (x >= 0 && x < width_ && y >= 0 && y < height_) {
        return tiles_[y * width_ + x];
    }
    return -1;
}

void Tilemap::fill(int tileId) {
    std::fill(tiles_.begin(), tiles_.end(), tileId);
}

void Tilemap::setTileset(TexturePtr tileset, int tilesPerRow) {
    tileset_ = tileset;
    tilesPerRow_ = tilesPerRow;
}

Vec2 Tilemap::worldToGrid(const Vec2& worldPos) const {
    Vec2 localPos = worldPos - position_;
    return Vec2{
        localPos.x / static_cast<float>(tileWidth_),
        localPos.y / static_cast<float>(tileHeight_)
    };
}

Vec2 Tilemap::gridToWorld(int x, int y) const {
    return Vec2{
        position_.x + x * tileWidth_,
        position_.y + y * tileHeight_
    };
}

int Tilemap::getTileAtWorldPos(const Vec2& worldPos) const {
    Vec2 gridPos = worldToGrid(worldPos);
    int x = static_cast<int>(gridPos.x);
    int y = static_cast<int>(gridPos.y);
    return getTile(x, y);
}

} // namespace Engine
