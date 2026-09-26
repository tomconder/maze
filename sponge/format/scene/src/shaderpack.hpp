#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Baked shader pack (shaders.spnga). Every compiled GLSL stage in one file,
// keyed by name. Written by tools/assetconv, read by Shader.
//
// Own magic and version, separate from the model container, so a change to
// one layout does not invalidate the other's baked files. magic and version
// stay at offsets 0 and 8, as in assetformat.hpp.
namespace sponge::scene::shaderpack {

constexpr char     magic[8] = { 'S', 'P', 'N', 'G', 'S', 'H', 0, 0 };
constexpr uint32_t version  = 1;

struct Header {
    char     magic[sizeof(shaderpack::magic)];
    uint32_t version;
    uint32_t count;
};

// Byte offsets are absolute from the start of the file.
struct Entry {
    uint32_t nameOffset;
    uint32_t nameSize;
    uint32_t sourceOffset;
    uint32_t sourceSize;
};

// read() and write() index the file with these sizes.
static_assert(sizeof(Header) == 16);
static_assert(sizeof(Entry) == 16);

using Sources = std::unordered_map<std::string, std::string>;

// Returns an empty vector if the pack would pass 4 GiB, the limit of the u32
// offsets.
std::vector<uint8_t> write(const Sources& sources);

// On failure, returns an empty map and sets error. error is left alone on
// success, so pass an empty string.
Sources read(const std::string& path, std::string& error);

}  // namespace sponge::scene::shaderpack
