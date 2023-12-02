#pragma once

#include "graphics/renderer/graphicsinfo.h"

namespace sponge::graphics::renderer {

class OpenGLInfo : public GraphicsInfo {
   public:
    static void logContextInfo();
    static void logGraphicsDriverInfo();
    static void logStaticInfo();
    static void logVersion();
};

}  // namespace sponge::graphics::renderer
