#pragma once

#include <glm/glm.hpp>

namespace sponge::platform::opengl::renderer {

class RendererAPI final {
public:
    void init() const;
    void setViewport(int32_t x, int32_t y, int32_t width, int32_t height) const;

    void setClearColor(const glm::vec4& color) const;
    void clear() const;
};

}  // namespace sponge::platform::opengl::renderer
