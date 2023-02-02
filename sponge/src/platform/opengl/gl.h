#pragma once

#ifdef _MSC_VER
#pragma warning(disable : 4005)
#endif

#ifdef EMSCRIPTEN
#define GLAD_GLES2_IMPLEMENTATION
#endif

#include <glad/glad.h>
#include <stdint.h>
