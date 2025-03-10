#pragma once

#if defined(ENABLE_PROFILING)

#include <glad/glad.h>
#include <tracy/TracyOpenGL.hpp>

#define SPONGE_PROFILE_GPU(text)   TracyGpuZone(text)
#define SPONGE_PROFILE_GPU_CONTEXT TracyGpuContext
#define SPONGE_PROFILE_GPU_COLLECT TracyGpuCollect

#else

#define SPONGE_PROFILE_GPU(text)
#define SPONGE_PROFILE_GPU_CONTEXT
#define SPONGE_PROFILE_GPU_COLLECT

#endif
