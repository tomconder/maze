#include "scene/format/shaderpack.hpp"

#include "scene/format/readbytes.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace sponge::scene::shaderpack {

Sources read(const std::string& path, std::string& error) {
    const auto bytes = readBytes(path);
    if (bytes.size() < sizeof(Header)) {
        error = fmt::format(
            "Unable to read shader pack, or it is truncated: {}", path);
        return {};
    }

    Header header{};
    std::memcpy(&header, bytes.data(), sizeof(Header));
    if (std::memcmp(header.magic, magic.data(), magic.size()) != 0) {
        error = fmt::format("Not a shader pack: {}", path);
        return {};
    }
    if (header.version != version) {
        error = fmt::format("Shader pack version {}, expected {}: {}",
                            header.version, version, path);
        return {};
    }
    if (bytes.size() < sizeof(Header) + (sizeof(Entry) * header.count)) {
        error = fmt::format("Shader pack entry table is truncated: {}", path);
        return {};
    }

    Sources sources;
    for (uint32_t i = 0; i < header.count; i++) {
        Entry entry{};
        std::memcpy(&entry, bytes.data() + sizeof(Header) + (sizeof(Entry) * i),
                    sizeof(Entry));
        if (uint64_t{ entry.nameOffset } + entry.nameSize > bytes.size() ||
            uint64_t{ entry.sourceOffset } + entry.sourceSize > bytes.size()) {
            error = fmt::format("Shader {} runs past the end of {}", i, path);
            return {};
        }
        const auto* base = reinterpret_cast<const char*>(bytes.data());
        sources.emplace(
            std::string{ base + entry.nameOffset, entry.nameSize },
            std::string{ base + entry.sourceOffset, entry.sourceSize });
    }
    return sources;
}

}  // namespace sponge::scene::shaderpack
