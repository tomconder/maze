#pragma once

#include "renderer/graphicsinfo.hpp"

namespace sponge::platform::opengl {

class OpenGLInfo : public renderer::GraphicsInfo {
   public:
    static void logContextInfo();
    static void logGraphicsDriverInfo();
    static void logStaticInfo();
    static void logVersion();
};

}  // namespace sponge::platform::opengl
