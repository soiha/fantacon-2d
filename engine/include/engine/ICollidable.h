#pragma once

#include "Types.h"

namespace Engine {

class GameObject;

// Interface for GameObjects that participate in collision detection
class ICollidable {
public:
    virtual ~ICollidable() = default;

    // Get axis-aligned bounding box for collision detection
    virtual Rect getBounds() const = 0;

    // Called when this object collides with another
    virtual void onCollision(GameObject* other) = 0;

    // Collision filtering (which layers this object is on and which it collides with)
    virtual int getCollisionLayer() const { return 0; }
    virtual int getCollisionMask() const { return 0xFFFFFFFF; }  // Collide with all layers by default
};

} // namespace Engine
