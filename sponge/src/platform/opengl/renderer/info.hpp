#pragma once

#include "renderer/graphicsinfo.hpp"

namespace sponge::platform::opengl::renderer {

class Info final : public sponge::renderer::GraphicsInfo {
   public:
    static void logContextInfo();
    static void logGraphicsDriverInfo();
    static void logStaticInfo();
    static void logVersion();
};

}  // namespace sponge::platform::opengl::renderer
