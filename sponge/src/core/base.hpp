#pragma once

#include "core/log.hpp"
#include <cstdint>

#define BIT(x)    (1 << (x))
#define UNUSED(x) (void)(x)

namespace sponge {

void startupCore();
void shutdownCore();

}  // namespace sponge