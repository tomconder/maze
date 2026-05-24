#pragma once

#ifdef ENABLE_PROFILING

#include <tracy/Tracy.hpp>

#define SPONGE_PROFILE                    ZoneScoped
#define SPONGE_PROFILE_FRAME(x)           FrameMark
#define SPONGE_PROFILE_SECTION(x)         ZoneScopedN(x)
#define SPONGE_PROFILE_TAG(y, x)          ZoneText(x, strlen(x))
#define SPONGE_PROFILE_LOG(text, size)    TracyMessage(text, size)
#define SPONGE_PROFILE_VALUE(text, value) TracyPlot(text, value)
#define SPONGE_PROFILE_ALLOC(ptr, size)   TracyAlloc(ptr, size)
#define SPONGE_PROFILE_FREE(ptr)          TracyFree(ptr)

#else

#define SPONGE_PROFILE                    ((void)0)
#define SPONGE_PROFILE_FRAME(x)           ((void)0)
#define SPONGE_PROFILE_SECTION(x)         ((void)0)
#define SPONGE_PROFILE_TAG(y, x)          ((void)0)
#define SPONGE_PROFILE_LOG(text, size)    ((void)0)
#define SPONGE_PROFILE_VALUE(text, value) ((void)0)
#define SPONGE_PROFILE_ALLOC(ptr, size)   ((void)0)
#define SPONGE_PROFILE_FREE(ptr)          ((void)0)

#endif
