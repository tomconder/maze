#pragma once

#include "renderer/rendererapi.hpp"
#include <glm/vec4.hpp>

namespace sponge::platform::opengl {

class OpenGLRendererAPI : public renderer::RendererAPI {
   public:
    void init() override;
    void setViewport(int32_t x, int32_t y, int32_t width,
                     int32_t height) override;

    void setClearColor(const glm::vec4& color) override;
    void clear() override;
};

}  // namespace sponge::platform::opengl
