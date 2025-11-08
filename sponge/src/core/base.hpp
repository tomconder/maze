#pragma once

#define BIT(x)    (1 << (x))
#define UNUSED(x) (void) (x)

#define GLFW_INCLUDE_NONE

namespace sponge::core {

void startupCore();
void shutdownCore();

}  // namespace sponge::core
