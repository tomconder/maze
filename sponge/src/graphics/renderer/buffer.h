#pragma once

namespace sponge::graphics::renderer {

class Buffer {
   public:
    virtual ~Buffer() = default;

    virtual void bind() const = 0;
};

}  // namespace sponge
