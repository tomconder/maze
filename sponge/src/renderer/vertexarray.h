#pragma once

namespace Sponge {

class VertexArray {
   public:
    virtual ~VertexArray() = default;

    virtual void bind() const = 0;
};

}  // namespace Sponge
