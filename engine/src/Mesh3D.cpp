#define _USE_MATH_DEFINES  // Must be before <cmath> for M_PI on Windows
#include "engine/Mesh3D.h"
#include "engine/IndexedPixelBuffer.h"
#include "engine/Sprite.h"
#include <cmath>

namespace Engine {

// Helper function to get anchor point position on a sprite
static Vec2 getAnchorPosition(const Sprite& sprite, AnchorPoint anchor) {
    Vec2 pos = sprite.getPosition();
    auto texture = sprite.getTexture();

    if (!texture) {
        return pos;
    }

    float width = static_cast<float>(texture->getWidth());
    float height = static_cast<float>(texture->getHeight());
    Vec2 scale = sprite.getScale();
    width *= scale.x;
    height *= scale.y;

    switch (anchor) {
        case AnchorPoint::TopLeft:
            return pos;
        case AnchorPoint::TopRight:
            return pos + Vec2{width, 0};
        case AnchorPoint::BottomLeft:
            return pos + Vec2{0, height};
        case AnchorPoint::BottomRight:
            return pos + Vec2{width, height};
        case AnchorPoint::Center:
            return pos + Vec2{width / 2.0f, height / 2.0f};
    }
    return pos;
}

Mesh3D::Mesh3D() {
    // Create a default-sized buffer (will be resized to viewport in ensureBuffer)
    buffer_ = std::make_shared<IndexedPixelBuffer>(640, 480);
}

Mesh3D::Mesh3D(int bufferWidth, int bufferHeight)
    : bufferWidth_(bufferWidth)
    , bufferHeight_(bufferHeight) {
    // Create buffer with specified dimensions or default size
    int width = (bufferWidth > 0) ? bufferWidth : 640;
    int height = (bufferHeight > 0) ? bufferHeight : 480;
    buffer_ = std::make_shared<IndexedPixelBuffer>(width, height);
}

void Mesh3D::addVertex(const Vec3& vertex) {
    vertices_.push_back(vertex);
}

void Mesh3D::addNormal(const Vec3& normal) {
    normals_.push_back(normal);
}

void Mesh3D::addPolygon(const Polygon& polygon) {
    polygons_.push_back(polygon);
}

void Mesh3D::clear() {
    vertices_.clear();
    normals_.clear();
    polygons_.clear();
}

void Mesh3D::setFOV(float degrees) {
    // Convert FOV to focal length
    // Reference distance of 600 pixels (typical screen height)
    const float referenceDistance = 600.0f;
    float radians = degrees * (M_PI / 180.0f);
    focalLength_ = referenceDistance / std::tan(radians / 2.0f);
}

float Mesh3D::getFOV() const {
    // Convert focal length back to FOV in degrees
    const float referenceDistance = 600.0f;
    float radians = 2.0f * std::atan(referenceDistance / focalLength_);
    return radians * (180.0f / M_PI);
}

void Mesh3D::anchorToSprite(std::shared_ptr<Sprite> sprite,
                             AnchorPoint spriteAnchor,
                             Vec2 offset) {
    anchoredSprite_ = sprite;
    spriteAnchor_ = spriteAnchor;
    anchorOffset_ = offset;
    anchorMode_ = MeshAnchorMode::Sprite;
}

void Mesh3D::clearSpriteAnchor() {
    anchoredSprite_.reset();
    anchorMode_ = MeshAnchorMode::Viewport;
    anchorOffset_ = Vec2{0.0f, 0.0f};
}

void Mesh3D::update(float deltaTime) {
    // Apply auto-rotation
    rotation_.x += autoRotation_.x * deltaTime;
    rotation_.y += autoRotation_.y * deltaTime;
    rotation_.z += autoRotation_.z * deltaTime;
}

void Mesh3D::setBufferSize(int width, int height) {
    bufferWidth_ = width;
    bufferHeight_ = height;
    buffer_.reset();  // Force recreation on next render
}

int Mesh3D::getBufferWidth() const {
    if (bufferWidth_ > 0) {
        return bufferWidth_;
    }
    return buffer_ ? buffer_->getWidth() : 0;
}

int Mesh3D::getBufferHeight() const {
    if (bufferHeight_ > 0) {
        return bufferHeight_;
    }
    return buffer_ ? buffer_->getHeight() : 0;
}

std::shared_ptr<IndexedPixelBuffer> Mesh3D::getBuffer() const {
    return buffer_;
}

void Mesh3D::ensureBuffer(IRenderer& renderer) {
    int width = bufferWidth_;
    int height = bufferHeight_;

    // 0 means use viewport size
    if (width == 0) {
        width = renderer.getViewportWidth();
    }
    if (height == 0) {
        height = renderer.getViewportHeight();
    }

    // Create buffer if it doesn't exist or size changed
    if (!buffer_ || buffer_->getWidth() != width || buffer_->getHeight() != height) {
        // Preserve palette if buffer exists
        std::array<Color, 256> palette;
        bool hasPalette = false;
        if (buffer_) {
            palette = buffer_->getPalette();
            hasPalette = true;
        }

        buffer_ = std::make_shared<IndexedPixelBuffer>(width, height);

        // Restore palette if we had one
        if (hasPalette) {
            buffer_->setPalette(palette);
        }
    }
}

void Mesh3D::render(IRenderer& renderer, const Vec2& layerOffset, float opacity) {
    if (!visible_) {
        return;
    }

    // Ensure internal buffer is ready
    ensureBuffer(renderer);

    // Clear buffer to transparent
    buffer_->clear(0);

    // Render mesh to internal buffer
    renderToBuffer(*buffer_);

    // Render buffer to screen via renderer
    renderer.renderIndexedPixelBuffer(*buffer_, layerOffset, opacity);
}

void Mesh3D::renderToBuffer(IndexedPixelBuffer& buffer) const {
    if (!visible_ || vertices_.empty() || polygons_.empty()) {
        return;
    }

    // Calculate final rendering position based on anchor mode
    Vec2 finalPosition = position_;
    if (anchorMode_ == MeshAnchorMode::Sprite) {
        auto sprite = anchoredSprite_.lock();
        if (sprite) {
            finalPosition = getAnchorPosition(*sprite, spriteAnchor_) + anchorOffset_;
        }
    }

    // Transform vertices
    std::vector<Vec3> transformed(vertices_.size());

    // Precompute rotation matrix components
    float cosX = std::cos(rotation_.x);
    float sinX = std::sin(rotation_.x);
    float cosY = std::cos(rotation_.y);
    float sinY = std::sin(rotation_.y);
    float cosZ = std::cos(rotation_.z);
    float sinZ = std::sin(rotation_.z);

    for (size_t i = 0; i < vertices_.size(); ++i) {
        Vec3 v = vertices_[i] * scale_;

        // Rotate around X axis
        float y1 = v.y * cosX - v.z * sinX;
        float z1 = v.y * sinX + v.z * cosX;

        // Rotate around Y axis
        float x2 = v.x * cosY - z1 * sinY;
        float z2 = v.x * sinY + z1 * cosY;

        // Rotate around Z axis
        float x3 = x2 * cosZ - y1 * sinZ;
        float y3 = x2 * sinZ + y1 * cosZ;

        transformed[i] = Vec3(x3, y3, z2);
    }

    // Transform normals (if we have them)
    // Normals are rotated but not scaled or translated
    std::vector<Vec3> transformedNormals(normals_.size());
    for (size_t i = 0; i < normals_.size(); ++i) {
        Vec3 n = normals_[i];

        // Rotate around X axis
        float y1 = n.y * cosX - n.z * sinX;
        float z1 = n.y * sinX + n.z * cosX;

        // Rotate around Y axis
        float x2 = n.x * cosY - z1 * sinY;
        float z2 = n.x * sinY + z1 * cosY;

        // Rotate around Z axis
        float x3 = x2 * cosZ - y1 * sinZ;
        float y3 = x2 * sinZ + y1 * cosZ;

        transformedNormals[i] = Vec3(x3, y3, z2);
        transformedNormals[i].normalize();
    }

    // View direction (camera looking down -Z axis)
    Vec3 viewDir(0, 0, -1);

    // Draw each polygon with back-face culling and lighting
    for (const auto& poly : polygons_) {
        if (poly.vertices.size() < 3) continue;

        // Calculate face normal
        Vec3 normal;
        if (!poly.normals.empty() && poly.normals[0] < transformedNormals.size()) {
            // Use the pre-loaded normal (first vertex's normal index)
            normal = transformedNormals[poly.normals[0]];
        } else {
            // Calculate face normal from vertices using cross product
            Vec3 v0 = transformed[poly.vertices[0]];
            Vec3 v1 = transformed[poly.vertices[1]];
            Vec3 v2 = transformed[poly.vertices[2]];

            Vec3 edge1 = v1 - v0;
            Vec3 edge2 = v2 - v0;
            normal = edge1.cross(edge2);
            normal.normalize();
        }

        // Back-face culling
        float lightIntensity = normal.dot(viewDir);
        if (backFaceCulling_ && lightIntensity <= 0) {
            continue;  // Back face, skip it
        }

        // Calculate lighting (light is at camera position)
        // Use the polygon's base color and modulate brightness based on lighting
        lightIntensity = std::max(0.0f, std::min(1.0f, lightIntensity));

        // Map light intensity to brightness level
        // Palette layout: 1-7 (bright), 8-14 (medium), 15-21 (dark)
        uint8_t baseColor = poly.color;
        uint8_t litColor;

        if (lightIntensity > 0.67f) {
            // High intensity: use bright color (original palette index 1-7)
            litColor = baseColor;
        } else if (lightIntensity > 0.33f) {
            // Medium intensity: use medium brightness (palette index +7)
            litColor = baseColor + 7;
        } else {
            // Low intensity: use dark color (palette index +14)
            litColor = baseColor + 14;
        }

        // Project all vertices for this polygon
        std::vector<Vec2> projected;
        projected.reserve(poly.vertices.size());

        for (int vIdx : poly.vertices) {
            Vec3 p = transformed[vIdx];
            float z = p.z + cameraDistance_;
            if (z < 1.0f) z = 1.0f;  // Avoid division by zero

            int x = static_cast<int>(finalPosition.x + (p.x * focalLength_) / z);
            int y = static_cast<int>(finalPosition.y + (p.y * focalLength_) / z);
            projected.push_back(Vec2{static_cast<float>(x), static_cast<float>(y)});
        }

        // Render based on mode
        if (renderMode_ == MeshRenderMode::Filled) {
            // Triangulate and fill the polygon
            // For triangles: draw directly
            // For quads+: fan triangulation from first vertex
            for (size_t i = 1; i + 1 < projected.size(); ++i) {
                buffer.fillTriangle(
                    static_cast<int>(projected[0].x), static_cast<int>(projected[0].y),
                    static_cast<int>(projected[i].x), static_cast<int>(projected[i].y),
                    static_cast<int>(projected[i + 1].x), static_cast<int>(projected[i + 1].y),
                    litColor
                );
            }
        } else {
            // Wireframe mode: draw edges only
            // Draw lines connecting all vertices in order, plus closing edge
            for (size_t i = 0; i < projected.size(); ++i) {
                size_t next = (i + 1) % projected.size();
                buffer.drawLine(
                    static_cast<int>(projected[i].x), static_cast<int>(projected[i].y),
                    static_cast<int>(projected[next].x), static_cast<int>(projected[next].y),
                    litColor
                );
            }
        }
    }
}

std::shared_ptr<Mesh3D> Mesh3D::createCube(float size) {
    auto mesh = std::make_shared<Mesh3D>();
    float s = size / 2.0f;

    // 8 vertices of a cube
    mesh->addVertex(Vec3(-s, -s, -s)); // 0
    mesh->addVertex(Vec3( s, -s, -s)); // 1
    mesh->addVertex(Vec3( s,  s, -s)); // 2
    mesh->addVertex(Vec3(-s,  s, -s)); // 3
    mesh->addVertex(Vec3(-s, -s,  s)); // 4
    mesh->addVertex(Vec3( s, -s,  s)); // 5
    mesh->addVertex(Vec3( s,  s,  s)); // 6
    mesh->addVertex(Vec3(-s,  s,  s)); // 7

    // 6 faces (CCW winding when viewed from outside)
    mesh->addPolygon(Polygon({0, 1, 2, 3}, 1)); // Front
    mesh->addPolygon(Polygon({5, 4, 7, 6}, 2)); // Back
    mesh->addPolygon(Polygon({4, 0, 3, 7}, 3)); // Left
    mesh->addPolygon(Polygon({1, 5, 6, 2}, 4)); // Right
    mesh->addPolygon(Polygon({3, 2, 6, 7}, 5)); // Top
    mesh->addPolygon(Polygon({4, 5, 1, 0}, 6)); // Bottom

    return mesh;
}

std::shared_ptr<Mesh3D> Mesh3D::createPyramid(float size) {
    auto mesh = std::make_shared<Mesh3D>();
    float s = size / 2.0f;

    // 5 vertices: 4 base corners + 1 apex
    mesh->addVertex(Vec3(-s, -s, -s));  // 0: base front-left
    mesh->addVertex(Vec3( s, -s, -s));  // 1: base front-right
    mesh->addVertex(Vec3( s, -s,  s));  // 2: base back-right
    mesh->addVertex(Vec3(-s, -s,  s));  // 3: base back-left
    mesh->addVertex(Vec3( 0,  s,  0));  // 4: apex

    // 5 faces
    mesh->addPolygon(Polygon({0, 1, 2, 3}, 1)); // Base
    mesh->addPolygon(Polygon({0, 4, 1}, 2));    // Front face
    mesh->addPolygon(Polygon({1, 4, 2}, 3));    // Right face
    mesh->addPolygon(Polygon({2, 4, 3}, 4));    // Back face
    mesh->addPolygon(Polygon({3, 4, 0}, 5));    // Left face

    return mesh;
}

std::shared_ptr<Mesh3D> Mesh3D::createSphere(float radius, int segments) {
    auto mesh = std::make_shared<Mesh3D>();

    // Create vertices
    for (int lat = 0; lat <= segments; ++lat) {
        float theta = lat * M_PI / segments;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (int lon = 0; lon <= segments; ++lon) {
            float phi = lon * 2.0f * M_PI / segments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;

            mesh->addVertex(Vec3(x * radius, y * radius, z * radius));
        }
    }

    // Create polygons
    for (int lat = 0; lat < segments; ++lat) {
        for (int lon = 0; lon < segments; ++lon) {
            int first = (lat * (segments + 1)) + lon;
            int second = first + segments + 1;

            // Create quad as two triangles
            uint8_t color = 1 + ((lat + lon) % 6);
            mesh->addPolygon(Polygon({first, second, first + 1}, color));
            mesh->addPolygon(Polygon({second, second + 1, first + 1}, color));
        }
    }

    return mesh;
}

} // namespace Engine
