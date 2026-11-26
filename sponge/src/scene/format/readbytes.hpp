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

// Exactly `size` bytes at `offset`. Returns empty on any failure, which
// includes a range that is not inside the file.
std::vector<uint8_t> readRange(const std::string& path, uint64_t offset,
                               std::size_t size);

}  // namespace sponge::scene
