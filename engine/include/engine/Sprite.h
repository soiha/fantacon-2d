#pragma once

#include "Types.h"
#include "Texture.h"
#include <memory>

namespace Engine {

class Sprite : public std::enable_shared_from_this<Sprite> {
public:
    Sprite() = default;
    Sprite(TexturePtr texture, Vec2 position = {0, 0});

    // Transform properties
    void setPosition(const Vec2& pos) { position_ = pos; }
    void setRotation(float degrees) { rotation_ = degrees; }
    void setScale(const Vec2& scale) { scale_ = scale; }
    void setScale(float scale) { scale_ = {scale, scale}; }

    const Vec2& getPosition() const { return position_; }
    float getRotation() const { return rotation_; }
    const Vec2& getScale() const { return scale_; }

    // Get layer position (includes parent chain and anchoring)
    Vec2 getLayerPosition() const;

    // Texture
    void setTexture(TexturePtr texture) { texture_ = texture; }
    TexturePtr getTexture() const { return texture_; }

    // Source rectangle (for sprite sheets)
    void setSourceRect(const Rect& rect) { sourceRect_ = rect; useSourceRect_ = true; }
    void clearSourceRect() { useSourceRect_ = false; }
    const Rect& getSourceRect() const { return sourceRect_; }
    bool hasSourceRect() const { return useSourceRect_; }

    // Visibility
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }

    // Anchoring
    // Anchor this sprite to a parent sprite
    // parentAnchor: which point on the parent to attach to
    // childAnchor: which point on this sprite aligns with the parent's anchor
    // offset: additional offset from the anchor point
    void setParent(std::shared_ptr<Sprite> parent,
                   AnchorPoint parentAnchor = AnchorPoint::Center,
                   AnchorPoint childAnchor = AnchorPoint::Center,
                   Vec2 offset = {0, 0});

    void clearParent();
    std::shared_ptr<Sprite> getParent() const { return parent_.lock(); }

private:
    // Calculate the offset for a given anchor point on this sprite
    Vec2 getAnchorPointOffset(AnchorPoint anchor) const;

    TexturePtr texture_;
    Vec2 position_{0, 0};
    Vec2 scale_{1.0f, 1.0f};
    float rotation_ = 0.0f;  // in degrees
    Rect sourceRect_{0, 0, 0, 0};
    bool useSourceRect_ = false;
    bool visible_ = true;

    // Anchoring to parent sprite
    std::weak_ptr<Sprite> parent_;
    AnchorPoint parentAnchor_ = AnchorPoint::Center;
    AnchorPoint childAnchor_ = AnchorPoint::Center;
    Vec2 anchorOffset_{0, 0};
};

} // namespace Engine
