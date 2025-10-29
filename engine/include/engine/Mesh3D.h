#pragma once

#include "Types.h"
#include "ILayerAttachable.h"
#include <vector>
#include <memory>
#include <cmath>

namespace Engine {

// Forward declarations
class Sprite;
class IndexedPixelBuffer;

// Anchor mode for 3D meshes
enum class MeshAnchorMode {
    Viewport,  // Anchor to viewport coordinates (default)
    Sprite     // Anchor to a sprite's position
};

// Render mode for 3D meshes
enum class MeshRenderMode {
    Filled,     // Filled polygons with lighting (default)
    Wireframe   // Line edges only (respects back-face culling setting)
};

// 3D vector with basic operations
struct Vec3 {
    float x, y, z;

    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }

    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    Vec3 operator*(float scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }

    Vec3 cross(const Vec3& other) const {
        return Vec3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    float dot(const Vec3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    void normalize() {
        float len = std::sqrt(x*x + y*y + z*z);
        if (len > 0.0001f) {
            x /= len;
            y /= len;
            z /= len;
        }
    }
};

// Polygon face (references vertex indices)
struct Polygon {
    std::vector<int> vertices;  // Vertex indices in CCW order
    std::vector<int> normals;   // Normal indices (optional, same size as vertices)
    uint8_t color;              // Palette color index

    Polygon() : color(1) {}
    Polygon(std::initializer_list<int> verts, uint8_t col)
        : vertices(verts), color(col) {}
};

// 3D wireframe mesh
// First-class renderable object that can be attached to layers
class Mesh3D : public ILayerAttachable {
public:
    Mesh3D();
    // Constructor with explicit buffer size
    Mesh3D(int bufferWidth, int bufferHeight);
    ~Mesh3D() = default;

    // Factory methods for common shapes
    static std::shared_ptr<Mesh3D> createCube(float size = 30.0f);
    static std::shared_ptr<Mesh3D> createPyramid(float size = 30.0f);
    static std::shared_ptr<Mesh3D> createSphere(float radius = 30.0f, int segments = 8);

    // Geometry
    void addVertex(const Vec3& vertex);
    void addNormal(const Vec3& normal);
    void addPolygon(const Polygon& polygon);
    void clear();

    const std::vector<Vec3>& getVertices() const { return vertices_; }
    const std::vector<Vec3>& getNormals() const { return normals_; }
    const std::vector<Polygon>& getPolygons() const { return polygons_; }
    bool hasNormals() const { return !normals_.empty(); }

    // Transform properties
    void setPosition(const Vec2& pos) { position_ = pos; }
    const Vec2& getPosition() const { return position_; }

    void setRotation(const Vec3& rot) { rotation_ = rot; }
    const Vec3& getRotation() const { return rotation_; }

    void setScale(float scale) { scale_ = scale; }
    float getScale() const { return scale_; }

    // Auto-rotation for animation
    void setAutoRotation(const Vec3& rotSpeed) { autoRotation_ = rotSpeed; }
    const Vec3& getAutoRotation() const { return autoRotation_; }

    // Perspective settings
    void setFocalLength(float focal) { focalLength_ = focal; }
    float getFocalLength() const { return focalLength_; }

    void setCameraDistance(float dist) { cameraDistance_ = dist; }
    float getCameraDistance() const { return cameraDistance_; }

    // FOV (Field of View) control - more intuitive than focal length
    // Smaller FOV (e.g., 45°) = telephoto/zoomed in, less distortion
    // Larger FOV (e.g., 90°) = wide angle, more distortion
    void setFOV(float degrees);
    float getFOV() const;

    // Buffer configuration
    void setBufferSize(int width, int height);
    int getBufferWidth() const;
    int getBufferHeight() const;
    std::shared_ptr<IndexedPixelBuffer> getBuffer() const;

    // Visibility
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const override { return visible_; }

    // ILayerAttachable interface
    void render(IRenderer& renderer, const Vec2& layerOffset, float opacity) override;

    // Backward compatibility: render to external buffer
    void renderToBuffer(IndexedPixelBuffer& buffer) const;

    // Back-face culling
    void setBackFaceCulling(bool enabled) { backFaceCulling_ = enabled; }
    bool getBackFaceCulling() const { return backFaceCulling_; }

    // Render mode (filled polygons vs wireframe)
    void setRenderMode(MeshRenderMode mode) { renderMode_ = mode; }
    MeshRenderMode getRenderMode() const { return renderMode_; }

    // Anchoring - attach mesh to viewport or sprite
    void setAnchorMode(MeshAnchorMode mode) { anchorMode_ = mode; }
    MeshAnchorMode getAnchorMode() const { return anchorMode_; }

    // Anchor to a sprite (automatically sets anchor mode to Sprite)
    // spriteAnchor: which point on the sprite to attach to
    // offset: additional offset from the sprite's anchor point
    void anchorToSprite(std::shared_ptr<Sprite> sprite,
                        AnchorPoint spriteAnchor = AnchorPoint::Center,
                        Vec2 offset = {0, 0});

    // Clear sprite anchoring (reverts to viewport mode)
    void clearSpriteAnchor();

    std::shared_ptr<Sprite> getAnchoredSprite() const { return anchoredSprite_.lock(); }

    // Update for animation (call each frame if using auto-rotation)
    void update(float deltaTime);

private:
    // Internal rendering buffer
    std::shared_ptr<IndexedPixelBuffer> buffer_;
    int bufferWidth_ = 0;   // 0 means "use viewport size"
    int bufferHeight_ = 0;  // 0 means "use viewport size"

    // Helper: ensure buffer is created and sized appropriately
    void ensureBuffer(IRenderer& renderer);
    std::vector<Vec3> vertices_;
    std::vector<Vec3> normals_;
    std::vector<Polygon> polygons_;

    Vec2 position_{0.0f, 0.0f};     // Screen position (or offset if anchored to sprite)
    Vec3 rotation_{0.0f, 0.0f, 0.0f}; // Rotation angles (radians)
    float scale_ = 1.0f;
    Vec3 autoRotation_{0.0f, 0.0f, 0.0f}; // Auto-rotation speed per second

    float focalLength_ = 1039.0f;   // Default ~60° FOV (600 / tan(30°))
    float cameraDistance_ = 200.0f;

    bool visible_ = true;
    bool backFaceCulling_ = true;
    MeshRenderMode renderMode_ = MeshRenderMode::Filled;

    // Anchoring
    MeshAnchorMode anchorMode_ = MeshAnchorMode::Viewport;
    std::weak_ptr<Sprite> anchoredSprite_;
    AnchorPoint spriteAnchor_ = AnchorPoint::Center;
    Vec2 anchorOffset_{0.0f, 0.0f};
};

using Mesh3DPtr = std::shared_ptr<Mesh3D>;

} // namespace Engine
