#pragma once

#include "renderer/graphicsinfo.hpp"

namespace sponge::renderer {

class OpenGLInfo : public GraphicsInfo {
   public:
    static void logContextInfo();
    static void logGraphicsDriverInfo();
    static void logStaticInfo();
    static void logVersion();
};

}  // namespace sponge::renderer
