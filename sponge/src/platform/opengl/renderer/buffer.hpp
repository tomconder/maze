#pragma once

#include <cstdint>

namespace sponge::platform::opengl::renderer {
class Buffer {
public:
    virtual ~Buffer() = default;

    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    uint32_t getId() const {
        return id;
    }

protected:
    uint32_t id = 0;
};

}  // namespace sponge::platform::opengl::renderer
