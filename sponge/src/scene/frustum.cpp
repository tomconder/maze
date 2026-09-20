#include "scene/frustum.hpp"

#include <limits>

namespace sponge::scene {

AABB transform(const AABB& box, const glm::mat4& matrix) {
    glm::vec3 newMin(std::numeric_limits<float>::max());
    glm::vec3 newMax(std::numeric_limits<float>::lowest());

    for (int i = 0; i < 8; i++) {
        const glm::vec3 corner((i & 1) != 0 ? box.max.x : box.min.x,
                               (i & 2) != 0 ? box.max.y : box.min.y,
                               (i & 4) != 0 ? box.max.z : box.min.z);
        const glm::vec3 world(matrix * glm::vec4(corner, 1.F));
        newMin = glm::min(newMin, world);
        newMax = glm::max(newMax, world);
    }

    return { newMin, newMax };
}

Frustum::Frustum(const glm::mat4& viewProj) {
    // glm matrices are column-major; "row i" of the plane algebra is
    // (m[0][i], m[1][i], m[2][i], m[3][i]).
    const auto row = [&viewProj](const int i) {
        return glm::vec4(viewProj[0][i], viewProj[1][i], viewProj[2][i],
                         viewProj[3][i]);
    };
    const glm::vec4 r0 = row(0);
    const glm::vec4 r1 = row(1);
    const glm::vec4 r2 = row(2);
    const glm::vec4 r3 = row(3);

    planes[0] = r3 + r0;  // left
    planes[1] = r3 - r0;  // right
    planes[2] = r3 + r1;  // bottom
    planes[3] = r3 - r1;  // top
    planes[4] = r3 + r2;  // near
    planes[5] = r3 - r2;  // far

    for (auto& plane : planes) {
        const float len = glm::length(glm::vec3(plane));
        if (len > 0.F) {
            plane /= len;
        }
    }
}

bool Frustum::intersects(const AABB& box) const {
    for (const auto& plane : planes) {
        const glm::vec3 normal(plane);
        const glm::vec3 positive{
            normal.x >= 0.F ? box.max.x : box.min.x,
            normal.y >= 0.F ? box.max.y : box.min.y,
            normal.z >= 0.F ? box.max.z : box.min.z,
        };
        if (glm::dot(normal, positive) + plane.w < 0.F) {
            return false;
        }
    }
    return true;
}

}  // namespace sponge::scene
