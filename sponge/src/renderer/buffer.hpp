#pragma once

namespace sponge::renderer {

class Buffer {
   public:
    virtual ~Buffer() = default;

    virtual void bind() const = 0;
};

}  // namespace sponge::renderer
