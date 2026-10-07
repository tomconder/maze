#pragma once

#include "modeldata.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace assetconv {

// Decodes a PNG or JPEG to RGBA8, choosing the codec from the magic bytes.
// Prints the reason to stderr and returns nullopt on failure.
std::optional<sponge::scene::ParsedImage>
    decodeImage(std::span<const uint8_t> bytes, const std::string& name);

}  // namespace assetconv
