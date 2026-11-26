#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

// Reads baked assets from the resource folder. When the folder has an asset
// pack, the pack is the only source. If not, each asset is a loose file. The
// choice is made at the first read and does not change.
namespace sponge::core::assets {

// The asset `name`, relative to the asset root, or at most maxBytes of it.
// Safe to call from several threads. On failure, returns empty and sets error.
// error is left alone on success, so pass an empty string.
std::vector<uint8_t>
    read(std::string_view name, std::string& error,
         std::size_t maxBytes = std::numeric_limits<std::size_t>::max());

}  // namespace sponge::core::assets
