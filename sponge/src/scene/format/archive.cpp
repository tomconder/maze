#include "scene/format/archive.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace sponge::scene::archive {

std::string normalizeName(const std::string_view name) {
    std::string result{ name };
    std::ranges::replace(result, '\\', '/');
    result.erase(0, std::min(result.find_first_not_of('/'), result.size()));
    if (result.contains(':')) {
        return {};
    }

    // Every part between separators, including one after a trailing '/'.
    std::string_view rest{ result };
    while (true) {
        const auto slash = rest.find('/');
        const auto part  = rest.substr(0, slash);
        if (part.empty() || part == "." || part == "..") {
            return {};
        }
        if (slash == std::string_view::npos) {
            return result;
        }
        rest.remove_prefix(slash + 1);
    }
}

std::string dataFileName(const uint32_t index) {
    return fmt::format("assets_{:03}.spndat", index);
}

Index read(const std::span<const uint8_t> bytes, std::string& error) {
    constexpr auto truncated = "The asset index is truncated";
    if (bytes.size() < sizeof(Header)) {
        error = truncated;
        return {};
    }

    Header header{};
    std::memcpy(&header, bytes.data(), sizeof(Header));
    if (std::memcmp(header.magic, magic.data(), magic.size()) != 0) {
        error = "Not an asset index";
        return {};
    }
    if (header.version != version) {
        error = fmt::format("Asset index version {}, expected {}",
                            header.version, version);
        return {};
    }

    // 32 bit counts times these sizes cannot overflow 64 bits.
    const uint64_t sizesEnd =
        sizeof(Header) + (sizeof(uint64_t) * uint64_t{ header.dataFileCount });
    const uint64_t entriesEnd =
        sizesEnd + (sizeof(Entry) * uint64_t{ header.entryCount });
    if (entriesEnd + header.namesSize > bytes.size()) {
        error = truncated;
        return {};
    }

    Index index;
    index.buildId = header.buildId;
    index.dataFileSizes.resize(header.dataFileCount);
    std::memcpy(index.dataFileSizes.data(), bytes.data() + sizeof(Header),
                sizeof(uint64_t) * header.dataFileCount);

    const std::string_view names{ reinterpret_cast<const char*>(bytes.data()) +
                                      entriesEnd,
                                  header.namesSize };
    std::string_view       last;
    for (uint32_t i = 0; i < header.entryCount; i++) {
        Entry entry{};
        std::memcpy(&entry, bytes.data() + sizesEnd + (sizeof(Entry) * i),
                    sizeof(Entry));
        if (uint64_t{ entry.nameOffset } + entry.nameSize > names.size()) {
            error = fmt::format("Asset {}: the name runs past the names", i);
            return {};
        }
        const auto  name = names.substr(entry.nameOffset, entry.nameSize);
        std::string owned{ name };
        if (name.empty() || normalizeName(name) != name) {
            error = fmt::format("Asset name is not normalized: {}", name);
            return {};
        }
        // Strictly ascending by bytes, which also rejects a repeated name.
        if (i > 0 && last >= name) {
            error = fmt::format("Asset name is out of order: {}", name);
            return {};
        }
        last = name;
        if (entry.dataFile >= header.dataFileCount) {
            error = fmt::format("{}: no data file {}", name, entry.dataFile);
            return {};
        }
        if (entry.compression != layout::ARCHIVE_COMPRESSION_NONE) {
            error = fmt::format("{}: unknown compression {}", name,
                                entry.compression);
            return {};
        }
        if (entry.size != entry.rawSize) {
            error = fmt::format("{}: the two sizes differ", name);
            return {};
        }
        const auto fileSize = index.dataFileSizes[entry.dataFile];
        if (entry.offset > fileSize || entry.size > fileSize - entry.offset) {
            error = fmt::format("{}: runs past the end of its data file", name);
            return {};
        }
        index.entries.emplace(
            std::move(owned),
            Location{ entry.dataFile, entry.offset, entry.size });
    }
    return index;
}

}  // namespace sponge::scene::archive
