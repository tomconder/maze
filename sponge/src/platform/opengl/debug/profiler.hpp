#pragma once

#include <tracy/TracyOpenGL.hpp>

#define SPONGE_PROFILE_GPU(text)   TracyGpuZone(text)
#define SPONGE_PROFILE_GPU_CONTEXT TracyGpuContext
#define SPONGE_PROFILE_GPU_COLLECT TracyGpuCollect
