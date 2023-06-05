#pragma once

namespace sponge {

class VertexArray {
   public:
    virtual ~VertexArray() = default;

    virtual void bind() const = 0;
};

}  // namespace sponge
