#pragma once

#include "renderer/graphicsinfo.hpp"

namespace sponge::platform::opengl::renderer {
class Info final : public sponge::renderer::GraphicsInfo {
   public:
    static void logInfo();
};
}  // namespace sponge::platform::opengl::renderer
