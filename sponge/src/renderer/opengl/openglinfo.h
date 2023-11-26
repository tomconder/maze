#pragma once

#include "renderer/graphicsinfo.h"

namespace sponge {

class OpenGLInfo : public GraphicsInfo {
   public:
    static void logContextInfo();
    static void logGraphicsDriverInfo();
    static void logStaticInfo();
    static void logVersion();
};

}  // namespace sponge
