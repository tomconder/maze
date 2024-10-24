#pragma once

#include <tracy/Tracy.hpp>

#define SPONGE_PROFILE                    ZoneScoped
#define SPONGE_PROFILE_FRAME(x)           FrameMark
#define SPONGE_PROFILE_SECTION(x)         ZoneScopedN(x)
#define SPONGE_PROFILE_TAG(y, x)          ZoneText(x, strlen(x))
#define SPONGE_PROFILE_LOG(text, size)    TracyMessage(text, size)
#define SPONGE_PROFILE_VALUE(text, value) TracyPlot(text, value)
