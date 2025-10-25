#pragma once

#include "glad/gl.h"

#include <cstdint>

void GLAPIENTRY glLogMessage(GLenum source, GLenum type, uint32_t id,
                             GLenum severity, GLsizei length,
                             const char* message, const void* userParam);
