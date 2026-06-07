#pragma once

#include <glm/glm.hpp>

namespace sponge::platform::opengl::renderer {

class RendererAPI final {
public:
    static void init();
    static void setViewport(int32_t x, int32_t y, int32_t width,
                            int32_t height);

    static void setClearColor(const glm::vec4& color);
    static void clear();
};

}  // namespace sponge::platform::opengl::renderer
