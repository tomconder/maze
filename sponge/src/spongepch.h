#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <memory>
#include <numeric>
#include <utility>

// string
#include <iostream>
#include <sstream>
#include <string>

// containers
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// glm
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// SDL
#include <SDL.h>

// spdlog
#if !NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#endif
#include <spdlog/spdlog.h>

// glad
#ifdef EMSCRIPTEN
#define GLAD_GLES2_IMPLEMENTATION
#endif

#include <glad/glad.h>
