#pragma once

#include "scene/format/assetlayout.generated.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// Asset pack: the index, assets.spnidx, and numbered data files. Written by
// tools/yeschef, read by core::assets. Each entry is one complete baked file.
// The byte layout is defined in Rust, in tools/yeschef/format, and reaches
// this code as assetlayout.generated.hpp.
namespace sponge::scene::archive {

static_assert(std::endian::native == std::endian::little);

using Header     = layout::ArchiveHeader;
using Entry      = layout::ArchiveEntry;
using DataHeader = layout::ArchiveDataHeader;

constexpr auto magic =
    std::bit_cast<std::array<uint8_t, 8>>(layout::ARCHIVE_MAGIC);
constexpr auto dataMagic =
    std::bit_cast<std::array<uint8_t, 8>>(layout::ARCHIVE_DATA_MAGIC);
constexpr uint32_t version = layout::ARCHIVE_VERSION;

// read() and the mount check index the files with these sizes.
static_assert(sizeof(Header) == 32);
static_assert(sizeof(Entry) == 40);
static_assert(sizeof(DataHeader) == 24);

inline constexpr std::string_view indexName = "assets.spnidx";

// Where one entry is.
struct Location {
    uint32_t dataFile;
    uint64_t offset;
    uint64_t size;
};

struct Index {
    uint64_t                                  buildId{ 0 };
    std::vector<uint64_t>                     dataFileSizes;
    std::unordered_map<std::string, Location> entries;
};

// The one spelling of an asset name: '/' separators and no leading '/'.
// Empty when the name is invalid.
std::string normalizeName(std::string_view name);

// The file name of data file `index`.
std::string dataFileName(uint32_t index);

// On failure, returns an empty Index and sets error. error is left alone on
// success, so pass an empty string.
Index read(std::span<const uint8_t> bytes, std::string& error);

}  // namespace sponge::scene::archive
