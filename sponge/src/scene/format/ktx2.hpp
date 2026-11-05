#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace sponge::scene::ktx2 {

// Subset of VkFormat, the values KTX2 stores in its vkFormat field.
enum Format : uint32_t {
    formatUndefined     = 0,
    formatR8G8B8Unorm   = 23,
    formatR8G8B8A8Unorm = 37,
    // Half float, for HDR images such as an environment map.
    formatR16G16B16A16Sfloat = 97,
    formatBc5Unorm           = 141,
    formatBc7Unorm           = 145,
};

// One mip level of a decoded KTX2 file, pointing into the caller's buffer.
struct Level {
    std::span<const uint8_t> bytes;
    uint32_t                 width{ 0 };
    uint32_t                 height{ 0 };
};

struct Image {
    Format             format{ formatUndefined };
    uint32_t           width{ 0 };
    uint32_t           height{ 0 };
    std::vector<Level> levels;
    // Key/value data, the container's own place for application metadata.
    // Values point into the caller's buffer, like Level::bytes.
    std::vector<std::pair<std::string, std::span<const uint8_t>>> keyValues;
};

// Parses a KTX2 file. Levels point into bytes, so bytes must outlive the
// result. On failure, returns an Image with no levels and sets error. error is
// left alone on success, so pass an empty string.
Image read(std::span<const uint8_t> bytes, std::string& error);

}  // namespace sponge::scene::ktx2
