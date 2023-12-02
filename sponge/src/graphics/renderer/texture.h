#pragma once

#include <string>

namespace sponge::graphics::renderer {

class Texture {
   public:
    virtual ~Texture() = default;

    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual uint32_t getId() const = 0;

    virtual void bind() const = 0;
};

}  // namespace sponge
