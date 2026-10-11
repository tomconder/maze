#pragma once

#include "platform/opengl/renderer/readback.hpp"

namespace game::capture {

// Writes a 0..1 float image as 8-bit RGB PNG. Values outside 0..1 clip, so
// use writePfm for radiance. Debug builds only: release builds do not build
// or link it.
bool writePng(const sponge::platform::opengl::renderer::Image& image,
              const char*                                      path);

}  // namespace game::capture
