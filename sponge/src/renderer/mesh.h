#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>

namespace Sponge {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
    glm::vec3 biTangent;
};

class Mesh {
   public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

}  // namespace Sponge
