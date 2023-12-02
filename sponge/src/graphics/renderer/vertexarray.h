#pragma once

namespace sponge::graphics::renderer {

class VertexArray {
   public:
    virtual ~VertexArray() = default;

    virtual void bind() const = 0;
};

}  // namespace sponge
