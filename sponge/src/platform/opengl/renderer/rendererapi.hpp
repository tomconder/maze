#pragma once

#include <glm/vec4.hpp>

namespace sponge::platform::opengl::renderer {

class RendererAPI final {
   public:
    void init();
    void setViewport(int32_t x, int32_t y, int32_t width, int32_t height);

    void setClearColor(const glm::vec4& color);
    void clear();
};

}  // namespace sponge::platform::opengl::renderer
