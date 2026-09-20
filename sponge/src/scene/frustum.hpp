#pragma once

#include <glm/glm.hpp>

#include <array>

namespace sponge::scene {

// Axis-aligned bounding box, in whatever space it was built in.
struct AABB {
    glm::vec3 min{ 0.F };
    glm::vec3 max{ 0.F };
};

// Re-derives a tight AABB after a transform: a rotated box's corners don't
// stay axis-aligned, so this re-expands from all 8, not just min/max.
AABB transform(const AABB& box, const glm::mat4& matrix);

// Camera frustum planes extracted from a combined view-projection matrix,
// normals pointing inward (Gribb/Hartmann method).
class Frustum {
public:
    explicit Frustum(const glm::mat4& viewProj);

    // Conservative: true if the box is inside or crosses the frustum, false
    // only once a plane fully rejects it. Never drops a box that's actually
    // visible; may keep one that's visible in bounds only, not in practice.
    bool intersects(const AABB& box) const;

private:
    // xyz = normal, w = distance; left/right/bottom/top/near/far.
    std::array<glm::vec4, 6> planes;
};

}  // namespace sponge::scene
