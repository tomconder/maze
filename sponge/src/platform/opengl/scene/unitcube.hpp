#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cstdint>

namespace sponge::platform::opengl::scene {

// Unit cube centred at the origin, extent [-0.5, 0.5] per axis. Shared by
// anything that just needs a cheap box proxy: light-position debug cubes,
// occlusion-query AABB proxies.
constexpr std::array unitCubeVertices = {
    glm::vec3{ -0.5, 0.5, -0.5 },  glm::vec3{ -0.5, 0.5, 0.5 },
    glm::vec3{ 0.5, 0.5, 0.5 },    glm::vec3{ -0.5, 0.5, -0.5 },
    glm::vec3{ 0.5, 0.5, 0.5 },    glm::vec3{ 0.5, 0.5, -0.5 },
    glm::vec3{ -0.5, 0.5, -0.5 },  glm::vec3{ -0.5, -0.5, -0.5 },
    glm::vec3{ -0.5, -0.5, 0.5 },  glm::vec3{ -0.5, 0.5, -0.5 },
    glm::vec3{ -0.5, -0.5, 0.5 },  glm::vec3{ -0.5, 0.5, 0.5 },
    glm::vec3{ 0.5, 0.5, 0.5 },    glm::vec3{ 0.5, -0.5, 0.5 },
    glm::vec3{ 0.5, -0.5, -0.5 },  glm::vec3{ 0.5, 0.5, 0.5 },
    glm::vec3{ 0.5, -0.5, -0.5 },  glm::vec3{ 0.5, 0.5, -0.5 },
    glm::vec3{ 0.5, 0.5, -0.5 },   glm::vec3{ 0.5, -0.5, -0.5 },
    glm::vec3{ -0.5, -0.5, -0.5 }, glm::vec3{ 0.5, 0.5, -0.5 },
    glm::vec3{ -0.5, -0.5, -0.5 }, glm::vec3{ -0.5, 0.5, -0.5 },
    glm::vec3{ -0.5, 0.5, 0.5 },   glm::vec3{ -0.5, -0.5, 0.5 },
    glm::vec3{ 0.5, -0.5, 0.5 },   glm::vec3{ -0.5, 0.5, 0.5 },
    glm::vec3{ 0.5, -0.5, 0.5 },   glm::vec3{ 0.5, 0.5, 0.5 },
    glm::vec3{ -0.5, -0.5, 0.5 },  glm::vec3{ -0.5, -0.5, -0.5 },
    glm::vec3{ 0.5, -0.5, -0.5 },  glm::vec3{ -0.5, -0.5, 0.5 },
    glm::vec3{ 0.5, -0.5, -0.5 },  glm::vec3{ 0.5, -0.5, 0.5 },
};
constexpr uint32_t unitCubeVertexCount = 36;

}  // namespace sponge::platform::opengl::scene
