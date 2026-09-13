#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace sponge::scene {

// Whole file as bytes, or at most maxBytes of it. Returns empty on any
// failure.
std::vector<uint8_t>
    readBytes(const std::string& path,
              std::size_t maxBytes = std::numeric_limits<std::size_t>::max());

}  // namespace sponge::scene
