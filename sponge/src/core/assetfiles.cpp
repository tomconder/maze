#include "core/assetfiles.hpp"

#include "core/file.hpp"
#include "logging/log.hpp"
#include "scene/format/archive.hpp"
#include "scene/format/readbytes.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {
namespace archive = sponge::scene::archive;

struct Mount {
    std::string folder;
    bool        packed{ false };
    // Only for a pack. A pack that fails to mount has error set.
    archive::Index index;
    std::string    error;
};

// Checks that every data file is present, has the recorded size and comes from
// the same build as the index.
std::string checkDataFiles(const std::string&    folder,
                           const archive::Index& index) {
    for (uint32_t i = 0; i < index.dataFileSizes.size(); i++) {
        const auto      path = folder + "/" + archive::dataFileName(i);
        std::error_code failure;
        const auto      size = std::filesystem::file_size(path, failure);
        if (failure) {
            return fmt::format("Unable to read asset data file: {}", path);
        }
        if (size != index.dataFileSizes[i]) {
            return fmt::format("Asset data file is {} bytes, expected {}: {}",
                               size, index.dataFileSizes[i], path);
        }

        const auto bytes =
            sponge::scene::readRange(path, 0, sizeof(archive::DataHeader));
        if (bytes.empty()) {
            return fmt::format("Unable to read asset data file: {}", path);
        }
        archive::DataHeader header{};
        std::memcpy(&header, bytes.data(), sizeof(header));
        if (std::memcmp(header.magic, archive::dataMagic.data(),
                        archive::dataMagic.size()) != 0 ||
            header.version != archive::version || header.dataFile != i) {
            return fmt::format("Not the expected asset data file: {}", path);
        }
        if (header.buildId != index.buildId) {
            return fmt::format(
                "Asset data file is from another build than the index: {}",
                path);
        }
    }
    return {};
}

Mount mount() {
    Mount      result{ .folder = sponge::core::File::getResourceDir() };
    const auto path = result.folder + "/" + std::string{ archive::indexName };

    // Loose files only when the index is certainly absent.
    std::error_code failure;
    const bool      found = std::filesystem::exists(path, failure);
    if (!found && !failure) {
        return result;
    }
    result.packed = true;

    const auto bytes = sponge::scene::readBytes(path);
    if (bytes.empty()) {
        result.error = fmt::format("Unable to read asset index: {}", path);
    } else {
        result.index = archive::read(bytes, result.error);
        if (result.error.empty()) {
            result.error = checkDataFiles(result.folder, result.index);
        } else {
            result.error = fmt::format("{}: {}", result.error, path);
        }
    }

    if (!result.error.empty()) {
        SPONGE_CORE_ERROR("Unable to mount the asset pack. {}", result.error);
    }
    return result;
}

// Mounted at the first read. C++ makes this initialization thread safe, and
// nothing changes the result afterwards.
const Mount& mounted() {
    static const Mount result = mount();
    return result;
}
}  // namespace

namespace sponge::core::assets {

std::vector<uint8_t> read(const std::string_view name, std::string& error,
                          const std::size_t maxBytes) {
    // Loose files and the pack take the same names.
    const auto assetName = archive::normalizeName(name);
    if (assetName.empty()) {
        error = fmt::format("Invalid asset name: {}", name);
        return {};
    }

    const auto& source = mounted();
    if (!source.packed) {
        const auto path  = source.folder + "/" + assetName;
        auto       bytes = scene::readBytes(path, maxBytes);
        if (bytes.empty()) {
            error = fmt::format("Unable to read asset: {}", path);
        }
        return bytes;
    }

    if (!source.error.empty()) {
        error = source.error;
        return {};
    }
    const auto entry = source.index.entries.find(assetName);
    if (entry == source.index.entries.end()) {
        error = fmt::format("The asset pack has no asset: {}", assetName);
        return {};
    }

    // Each call opens the data file, because models load on several threads.
    const auto& location = entry->second;
    const auto  size =
        static_cast<std::size_t>(std::min<uint64_t>(location.size, maxBytes));
    auto bytes = scene::readRange(source.folder + "/" +
                                      archive::dataFileName(location.dataFile),
                                  location.offset, size);
    if (bytes.empty()) {
        error =
            fmt::format("Unable to read asset from the pack: {}", assetName);
    }
    return bytes;
}

}  // namespace sponge::core::assets
