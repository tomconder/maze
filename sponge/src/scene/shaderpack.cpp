#include "scene/shaderpack.hpp"

#include "core/file.hpp"
#include "logging/log.hpp"

#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace sponge::scene::shaderpack {

std::vector<uint8_t> write(const Sources& sources) {
    // Sorted, so the same shaders always bake to the same bytes.
    const std::map<std::string, std::string> sorted{ sources.begin(),
                                                     sources.end() };

    Header header{
        .magic   = {},
        .version = version,
        .count   = static_cast<uint32_t>(sorted.size()),
    };
    std::memcpy(header.magic, magic, sizeof(magic));

    std::vector<uint8_t> out(sizeof(Header) + (sizeof(Entry) * sorted.size()));
    std::vector<Entry>   entries(sorted.size());

    const auto appendString = [&out](const std::string& s) {
        const auto offset = static_cast<uint32_t>(out.size());
        out.insert(out.end(), s.begin(), s.end());
        return offset;
    };

    auto entry = entries.begin();
    for (const auto& [name, source] : sorted) {
        entry->nameOffset   = appendString(name);
        entry->nameSize     = static_cast<uint32_t>(name.size());
        entry->sourceOffset = appendString(source);
        entry->sourceSize   = static_cast<uint32_t>(source.size());
        entry++;
    }

    std::memcpy(out.data(), &header, sizeof(Header));
    std::memcpy(out.data() + sizeof(Header), entries.data(),
                sizeof(Entry) * entries.size());
    return out;
}

Sources read(const std::string& path) {
    const auto bytes = core::File::readBytes(path);
    if (bytes.size() < sizeof(Header)) {
        SPONGE_ERROR("Unable to read shader pack, or it is truncated: {}",
                     path);
        return {};
    }

    Header header{};
    std::memcpy(&header, bytes.data(), sizeof(Header));
    if (std::memcmp(header.magic, magic, sizeof(magic)) != 0) {
        SPONGE_ERROR("Not a shader pack: {}", path);
        return {};
    }
    if (header.version != version) {
        SPONGE_ERROR("Shader pack version {}, expected {}: {}", header.version,
                     version, path);
        return {};
    }
    if (bytes.size() < sizeof(Header) + (sizeof(Entry) * header.count)) {
        SPONGE_ERROR("Shader pack entry table is truncated: {}", path);
        return {};
    }

    Sources sources;
    for (uint32_t i = 0; i < header.count; i++) {
        Entry entry{};
        std::memcpy(&entry, bytes.data() + sizeof(Header) + (sizeof(Entry) * i),
                    sizeof(Entry));
        if (uint64_t{ entry.nameOffset } + entry.nameSize > bytes.size() ||
            uint64_t{ entry.sourceOffset } + entry.sourceSize > bytes.size()) {
            SPONGE_ERROR("Shader {} runs past the end of {}", i, path);
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
