#pragma once

#include "vertex.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sponge::scene {
class Mesh {
public:
    size_t getNumIndices() const {
        return numIndices;
    }
    size_t getNumVertices() const {
        return numVertices;
    }

protected:
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
    size_t                numIndices  = 0;
    size_t                numVertices = 0;
};

}  // namespace sponge::scene
