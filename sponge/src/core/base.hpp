#pragma once

#define BIT(x)    (1 << (x))
#define UNUSED(x) (void)(x)

namespace sponge::core {

void startupCore();
void shutdownCore();

}  // namespace sponge::core
