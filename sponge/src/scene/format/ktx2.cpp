#include "scene/format/ktx2.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <ranges>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr uint8_t identifier[12] = { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32,
                                     0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };

// Fixed part of a KTX2 file: 12-byte identifier, 9 u32 header fields, then
// the index (4 u32 + 2 u64). The level index follows at this offset.
constexpr size_t headerSize     = 80;
constexpr size_t levelEntrySize = 24;

// Byte offsets of the header fields this reader uses.
constexpr size_t supercompressionField = 44;
constexpr size_t kvdOffsetField        = 56;
constexpr size_t kvdLengthField        = 60;

// Key/value entries are each a u32 length, then a NUL-terminated key, then
// the value, then padding to the next 4-byte boundary.
constexpr size_t kvAlignment = 4;

size_t padTo4(const size_t size) {
    return (size + kvAlignment - 1) / kvAlignment * kvAlignment;
}

uint32_t get32(const std::span<const uint8_t> bytes, const size_t offset) {
    uint32_t value = 0;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

uint64_t get64(const std::span<const uint8_t> bytes, const size_t offset) {
    uint64_t value = 0;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}
}  // namespace

namespace sponge::scene::ktx2 {

Image read(const std::span<const uint8_t> bytes, std::string& error) {
    if (bytes.size() < headerSize ||
        !std::equal(std::begin(identifier), std::end(identifier),
                    bytes.begin())) {
        error = fmt::format("Not a KTX2 file");
        return {};
    }

    const auto supercompression = get32(bytes, supercompressionField);
    if (supercompression != 0) {
        error = fmt::format("KTX2 supercompression scheme {} is not supported",
                            supercompression);
        return {};
    }

    Image image;
    image.format = static_cast<Format>(get32(bytes, 12));
    image.width  = get32(bytes, 20);
    image.height = get32(bytes, 24);

    const auto levelCount = std::max(get32(bytes, 40), 1U);
    if (bytes.size() < headerSize + (levelEntrySize * levelCount)) {
        error = fmt::format("KTX2 level index is truncated");
        return {};
    }

    const auto kvdOffset = get32(bytes, kvdOffsetField);
    const auto kvdLength = get32(bytes, kvdLengthField);
    if (kvdOffset + kvdLength > bytes.size()) {
        error =
            fmt::format("KTX2 key/value data runs past the end of the file");
        return {};
    }

    for (size_t at = kvdOffset;
         at + sizeof(uint32_t) <= kvdOffset + kvdLength;) {
        const auto entryLength = get32(bytes, at);
        const auto entry       = bytes.subspan(
            at + sizeof(uint32_t),
            std::min<size_t>(entryLength,
                             kvdOffset + kvdLength - at - sizeof(uint32_t)));
        const auto nul = std::ranges::find(entry, uint8_t{ 0 });
        if (nul == entry.end()) {
            error = fmt::format("KTX2 key/value entry has no key terminator");
            return {};
        }
        const auto keyLength = static_cast<size_t>(nul - entry.begin());
        image.keyValues.emplace_back(
            std::string(reinterpret_cast<const char*>(entry.data()), keyLength),
            entry.subspan(keyLength + 1));
        at += padTo4(sizeof(uint32_t) + entryLength);
    }

    image.levels.reserve(levelCount);
    for (uint32_t i = 0; i < levelCount; i++) {
        const auto entry  = headerSize + (levelEntrySize * i);
        const auto offset = get64(bytes, entry);
        const auto length = get64(bytes, entry + 8);
        if (offset + length > bytes.size()) {
            error =
                fmt::format("KTX2 level {} runs past the end of the file", i);
            return {};
        }
        image.levels.emplace_back(Level{
            .bytes  = bytes.subspan(offset, length),
            .width  = std::max(image.width >> i, 1U),
            .height = std::max(image.height >> i, 1U),
        });
    }

    return image;
}

}  // namespace sponge::scene::ktx2
