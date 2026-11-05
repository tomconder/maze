#pragma once

#include "scene/format/assetlayout.generated.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Baked shader pack (shaders.spnga). Every compiled GLSL stage in one file,
// keyed by name. Written by tools/yeschef, read by Shader.
//
// Own magic and version, separate from the model container, so a change to
// one layout does not invalidate the other's baked files. magic and version
// stay at offsets 0 and 8, as in assetformat.hpp. The byte layout is defined
// in Rust, in tools/yeschef/format, and reaches this code as
// assetlayout.generated.hpp.
namespace sponge::scene::shaderpack {

static_assert(std::endian::native == std::endian::little);

using Header = layout::ShaderPackHeader;
using Entry  = layout::ShaderPackEntry;

constexpr auto magic =
    std::bit_cast<std::array<uint8_t, 8>>(layout::SHADERPACK_MAGIC);
constexpr uint32_t version = layout::SHADERPACK_VERSION;

// read() indexes the file with these sizes.
static_assert(sizeof(Header) == 16);
static_assert(sizeof(Entry) == 16);

using Sources = std::unordered_map<std::string, std::string>;

// On failure, returns an empty map and sets error. error is left alone on
// success, so pass an empty string.
Sources read(const std::string& path, std::string& error);

}  // namespace sponge::scene::shaderpack
