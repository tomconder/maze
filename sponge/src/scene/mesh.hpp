#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace sponge::scene {
struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoords;
    glm::vec3 normal;
};

class Mesh {
public:
    void optimize();
    size_t getNumIndices() const {
        return numIndices;
    }
    size_t getNumVertices() const {
        return numVertices;
    }

protected:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    size_t numIndices = 0;
    size_t numVertices = 0;
};

}  // namespace sponge::scene
