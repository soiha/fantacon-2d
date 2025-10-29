#pragma once

#include "Sprite.h"
#include "Tilemap.h"
#include "PixelBuffer.h"
#include "IndexedPixelBuffer.h"
#include "Mesh3D.h"
#include "AttributedTextGrid.h"
#include "ILayerAttachable.h"
#include <vector>
#include <memory>

namespace Engine {

class IRenderer;

class Layer {
public:
    explicit Layer(int renderOrder = 0);

    // Render order
    void setRenderOrder(int order) { renderOrder_ = order; }
    int getRenderOrder() const { return renderOrder_; }

    // Sprite management
    void addSprite(const std::shared_ptr<Sprite>& sprite);
    void removeSprite(const std::shared_ptr<Sprite>& sprite);
    void clearSprites();
    const std::vector<std::shared_ptr<Sprite>>& getSprites() const { return sprites_; }

    // Tilemap management
    void addTilemap(const std::shared_ptr<Tilemap>& tilemap);
    void removeTilemap(const std::shared_ptr<Tilemap>& tilemap);
    void clearTilemaps();
    const std::vector<std::shared_ptr<Tilemap>>& getTilemaps() const { return tilemaps_; }

    // PixelBuffer management
    void addPixelBuffer(const std::shared_ptr<PixelBuffer>& buffer);
    void removePixelBuffer(const std::shared_ptr<PixelBuffer>& buffer);
    void clearPixelBuffers();
    const std::vector<std::shared_ptr<PixelBuffer>>& getPixelBuffers() const { return pixelBuffers_; }

    // IndexedPixelBuffer management
    void addIndexedPixelBuffer(const std::shared_ptr<IndexedPixelBuffer>& buffer);
    void removeIndexedPixelBuffer(const std::shared_ptr<IndexedPixelBuffer>& buffer);
    void clearIndexedPixelBuffers();
    const std::vector<std::shared_ptr<IndexedPixelBuffer>>& getIndexedPixelBuffers() const { return indexedPixelBuffers_; }

    // Mesh3D management
    void addMesh3D(const std::shared_ptr<Mesh3D>& mesh);
    void removeMesh3D(const std::shared_ptr<Mesh3D>& mesh);
    void clearMeshes();
    const std::vector<std::shared_ptr<Mesh3D>>& getMeshes() const { return meshes_; }

    // Render all meshes to a buffer (call before rendering the layer)
    void renderMeshes(class IndexedPixelBuffer& buffer);

    // AttributedTextGrid management
    void addTextGrid(const std::shared_ptr<AttributedTextGrid>& textGrid);
    void removeTextGrid(const std::shared_ptr<AttributedTextGrid>& textGrid);
    void clearTextGrids();
    const std::vector<std::shared_ptr<AttributedTextGrid>>& getTextGrids() const { return textGrids_; }

    // Render all text grids to a buffer (call before rendering the layer)
    void renderTextGrids(class IndexedPixelBuffer& buffer);

    // ILayerAttachable management (new unified API)
    void addAttachable(const std::shared_ptr<ILayerAttachable>& attachable);
    void removeAttachable(const std::shared_ptr<ILayerAttachable>& attachable);
    void clearAttachables();
    const std::vector<std::shared_ptr<ILayerAttachable>>& getAttachables() const { return attachables_; }

    // Visibility
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }

    // Opacity (0.0 = fully transparent, 1.0 = fully opaque)
    void setOpacity(float opacity) { opacity_ = std::max(0.0f, std::min(1.0f, opacity)); }
    float getOpacity() const { return opacity_; }

    // Layer offset (for camera, scrolling, etc.)
    void setOffset(const Vec2& offset) { offset_ = offset; }
    const Vec2& getOffset() const { return offset_; }
    void moveOffset(const Vec2& delta) { offset_ += delta; }

    // Rendering
    void render(IRenderer& renderer);

private:
    int renderOrder_;
    Vec2 offset_{0.0f, 0.0f};
    std::vector<std::shared_ptr<Sprite>> sprites_;
    std::vector<std::shared_ptr<Tilemap>> tilemaps_;
    std::vector<std::shared_ptr<PixelBuffer>> pixelBuffers_;
    std::vector<std::shared_ptr<IndexedPixelBuffer>> indexedPixelBuffers_;
    std::vector<std::shared_ptr<Mesh3D>> meshes_;
    std::vector<std::shared_ptr<AttributedTextGrid>> textGrids_;
    std::vector<std::shared_ptr<ILayerAttachable>> attachables_;  // New unified API
    bool visible_ = true;
    float opacity_ = 1.0f;  // 0.0 = fully transparent, 1.0 = fully opaque
};

} // namespace Engine
