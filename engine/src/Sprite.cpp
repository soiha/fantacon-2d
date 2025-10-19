#include "engine/Sprite.h"

namespace Engine {

Sprite::Sprite(TexturePtr texture, Vec2 position)
    : texture_(texture)
    , position_(position) {
}

Vec2 Sprite::getAnchorPointOffset(AnchorPoint anchor) const {
    if (!texture_) {
        return {0, 0};
    }

    float width = texture_->getWidth() * scale_.x;
    float height = texture_->getHeight() * scale_.y;

    switch (anchor) {
        case AnchorPoint::TopLeft:
            return {0, 0};
        case AnchorPoint::TopRight:
            return {width, 0};
        case AnchorPoint::BottomLeft:
            return {0, height};
        case AnchorPoint::BottomRight:
            return {width, height};
        case AnchorPoint::Center:
            return {width / 2.0f, height / 2.0f};
    }
    return {0, 0};
}

Vec2 Sprite::getLayerPosition() const {
    // If no parent, just return local position
    auto parent = parent_.lock();
    if (!parent) {
        return position_;
    }

    // Calculate parent's layer position (recursive for parent chains)
    Vec2 parentLayerPos = parent->getLayerPosition();

    // Add parent's anchor point offset
    Vec2 parentAnchorOffset = parent->getAnchorPointOffset(parentAnchor_);

    // Subtract our child anchor point offset (so that point aligns with parent)
    Vec2 childAnchorOffset = getAnchorPointOffset(childAnchor_);

    // Final position = parent layer pos + parent anchor + offset - child anchor + local position
    return parentLayerPos + parentAnchorOffset + anchorOffset_ - childAnchorOffset + position_;
}

void Sprite::setParent(std::shared_ptr<Sprite> parent,
                       AnchorPoint parentAnchor,
                       AnchorPoint childAnchor,
                       Vec2 offset) {
    parent_ = parent;
    parentAnchor_ = parentAnchor;
    childAnchor_ = childAnchor;
    anchorOffset_ = offset;
}

void Sprite::clearParent() {
    parent_.reset();
}

} // namespace Engine
