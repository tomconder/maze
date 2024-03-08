#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

namespace sponge::renderer {
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

class Mesh {
   public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    void optimize();
};

}  // namespace sponge::renderer
