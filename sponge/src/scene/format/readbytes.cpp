#include "scene/format/readbytes.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

namespace sponge::scene {

std::vector<uint8_t> readBytes(const std::string& path,
                               const std::size_t  maxBytes) {
    std::ifstream file{ path, std::ios::binary | std::ios::ate };
    if (!file) {
        return {};
    }

    const auto size =
        std::min(static_cast<std::size_t>(file.tellg()), maxBytes);
    file.seekg(0);
    std::vector<uint8_t> bytes(size);
    if (!file.read(reinterpret_cast<char*>(bytes.data()),
                   static_cast<std::streamsize>(size))) {
        return {};
    }
    return bytes;
}

std::vector<uint8_t> readRange(const std::string& path, const uint64_t offset,
                               const std::size_t size) {
    std::ifstream file{ path, std::ios::binary | std::ios::ate };
    if (!file) {
        return {};
    }

    // Checked before the allocation, in a form that cannot overflow.
    const std::streamoff end = file.tellg();
    if (end < 0) {
        return {};
    }
    const auto fileSize = static_cast<uint64_t>(end);
    if (offset > fileSize || size > fileSize - offset) {
        return {};
    }

    file.seekg(static_cast<std::streamoff>(offset));
    std::vector<uint8_t> bytes(size);
    if (!file.read(reinterpret_cast<char*>(bytes.data()),
                   static_cast<std::streamsize>(size))) {
        return {};
    }
    return bytes;
}

}  // namespace sponge::scene
