#pragma once

#ifdef ENABLE_PROFILING

#include <tracy/Tracy.hpp>

#define SPONGE_PROFILE                  ZoneScoped
#define SPONGE_PROFILE_SECTION(x)       ZoneScopedN(x)
#define SPONGE_PROFILE_ALLOC(ptr, size) TracyAlloc(ptr, size)
#define SPONGE_PROFILE_FREE(ptr)        TracyFree(ptr)

#else

#define SPONGE_PROFILE
#define SPONGE_PROFILE_SECTION(x)
#define SPONGE_PROFILE_ALLOC(ptr, size)
#define SPONGE_PROFILE_FREE(ptr)

#endif
