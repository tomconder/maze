#pragma once

#include "glm/vec4.hpp"
#include "graphics/renderer/rendererapi.h"

namespace sponge::graphics::renderer {

class OpenGLRendererAPI : public RendererAPI {
   public:
    void init() override;
    void setViewport(int32_t x, int32_t y, int32_t width,
                     int32_t height) override;

    void setClearColor(const glm::vec4& color) override;
    void clear() override;
};

}  // namespace sponge::graphics::renderer
