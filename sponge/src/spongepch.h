#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
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
#include <vector>

// glm
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// abseil
#include <absl/container/flat_hash_map.h>

// SDL
#include <SDL.h>

// spdlog
#if !NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#endif
#include <spdlog/spdlog.h>

// tuplet
#include <tuplet/tuple.hpp>

// gl bindings
#include <glad/glad.h>
