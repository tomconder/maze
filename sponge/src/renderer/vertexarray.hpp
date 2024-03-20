#pragma once

namespace sponge::renderer {

class VertexArray {
   public:
    virtual ~VertexArray() = default;

    virtual void bind() const = 0;
};

}  // namespace sponge::renderer
