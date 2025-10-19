#pragma once

#include "Types.h"
#include "Texture.h"
#include <vector>
#include <memory>

namespace Engine {

class Tilemap {
public:
    Tilemap(int width, int height, int tileWidth, int tileHeight);

    // Tile data (grid coordinates)
    void setTile(int x, int y, int tileId);
    int getTile(int x, int y) const;
    void fill(int tileId);

    // Coordinate conversion helpers
    // Convert world/layer position to grid coordinates
    Vec2 worldToGrid(const Vec2& worldPos) const;
    // Convert grid coordinates to world/layer position (top-left of tile)
    Vec2 gridToWorld(int x, int y) const;
    // Get tile at world/layer position
    int getTileAtWorldPos(const Vec2& worldPos) const;

    // Tileset
    void setTileset(TexturePtr tileset, int tilesPerRow);
    TexturePtr getTileset() const { return tileset_; }
    int getTilesPerRow() const { return tilesPerRow_; }

    // Position
    void setPosition(const Vec2& pos) { position_ = pos; }
    const Vec2& getPosition() const { return position_; }

    // Dimensions
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    int getTileWidth() const { return tileWidth_; }
    int getTileHeight() const { return tileHeight_; }

    // Visibility
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }

private:
    int width_;          // in tiles
    int height_;         // in tiles
    int tileWidth_;      // in pixels
    int tileHeight_;     // in pixels
    std::vector<int> tiles_;
    TexturePtr tileset_;
    int tilesPerRow_ = 0;
    Vec2 position_{0, 0};
    bool visible_ = true;
};

} // namespace Engine
