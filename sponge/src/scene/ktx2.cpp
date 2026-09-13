#include "scene/ktx2.hpp"

#include "logging/log.hpp"

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

// Byte offsets of the header fields this reader uses. The fixed header is
// the 12-byte identifier, nine u32s, then the index: four u32s and two u64s.
constexpr size_t supercompressionField = 44;
constexpr size_t kvdOffsetField        = 56;
constexpr size_t kvdLengthField        = 60;

// Key/value entries are each a u32 length, then a NUL-terminated key, then
// the value, then padding to the next 4-byte boundary.
constexpr size_t kvAlignment = 4;

size_t padTo4(const size_t size) {
    return (size + kvAlignment - 1) / kvAlignment * kvAlignment;
}

void put32(std::vector<uint8_t>& out, const uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        out.push_back(static_cast<uint8_t>(value >> shift));
    }
}

void put64(std::vector<uint8_t>& out, const uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8) {
        out.push_back(static_cast<uint8_t>(value >> shift));
    }
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

std::vector<uint8_t> write(const Format format, const uint32_t width,
                           const uint32_t                              height,
                           const std::span<const std::vector<uint8_t>> levels,
                           const std::span<const KeyValue> keyValues) {
    const auto levelCount = levels.size();

    // Layout: header, level index, key/value data, then level data largest
    // first. Offsets are absolute from the start of the file.
    size_t kvdSize = 0;
    for (const auto& [key, value] : keyValues) {
        kvdSize += padTo4(sizeof(uint32_t) + key.size() + 1 + value.size());
    }
    const auto kvdOffset =
        kvdSize == 0 ? 0 : headerSize + (levelEntrySize * levelCount);

    std::vector<uint64_t> offsets(levelCount);
    auto cursor = headerSize + (levelEntrySize * levelCount) + kvdSize;
    for (size_t i = 0; i < levelCount; i++) {
        offsets[i] = cursor;
        cursor += levels[i].size();
    }

    std::vector<uint8_t> out;
    out.reserve(cursor);
    out.insert(out.end(), std::begin(identifier), std::end(identifier));

    put32(out, format);
    // typeSize: one channel for the uncompressed formats here, and 1 by
    // definition for block-compressed ones. Every format we write is one byte
    // per channel either way.
    put32(out, 1);
    put32(out, width);
    put32(out, height);
    put32(out, 0);  // pixelDepth
    put32(out, 0);  // layerCount
    put32(out, 1);  // faceCount
    put32(out, static_cast<uint32_t>(levelCount));
    put32(out, 0);  // supercompressionScheme: none

    put32(out, 0);  // dfdByteOffset
    put32(out, 0);  // dfdByteLength
    put32(out, static_cast<uint32_t>(kvdOffset));
    put32(out, static_cast<uint32_t>(kvdSize));
    put64(out, 0);  // sgdByteOffset
    put64(out, 0);  // sgdByteLength

    for (size_t i = 0; i < levelCount; i++) {
        put64(out, offsets[i]);
        put64(out, levels[i].size());
        put64(out, levels[i].size());  // uncompressedByteLength
    }

    for (const auto& [key, value] : keyValues) {
        const auto entry = sizeof(uint32_t) + key.size() + 1 + value.size();
        put32(out, static_cast<uint32_t>(key.size() + 1 + value.size()));
        out.insert(out.end(), key.begin(), key.end());
        out.push_back(0);
        out.insert(out.end(), value.begin(), value.end());
        out.resize(out.size() + padTo4(entry) - entry);
    }

    for (const auto& level : levels) {
        out.insert(out.end(), level.begin(), level.end());
    }

    return out;
}

std::vector<uint8_t> write(const Format format, const uint32_t width,
                           const uint32_t                  height,
                           const std::span<const uint8_t>  pixels,
                           const std::span<const KeyValue> keyValues) {
    const std::vector<std::vector<uint8_t>> levels{ std::vector<uint8_t>{
        pixels.begin(), pixels.end() } };
    return write(format, width, height, levels, keyValues);
}

Image read(const std::span<const uint8_t> bytes) {
    if (bytes.size() < headerSize ||
        !std::equal(std::begin(identifier), std::end(identifier),
                    bytes.begin())) {
        SPONGE_ERROR("Not a KTX2 file");
        return {};
    }

    const auto supercompression = get32(bytes, supercompressionField);
    if (supercompression != 0) {
        SPONGE_ERROR("KTX2 supercompression scheme {} is not supported",
                     supercompression);
        return {};
    }

    Image image;
    image.format = static_cast<Format>(get32(bytes, 12));
    image.width  = get32(bytes, 20);
    image.height = get32(bytes, 24);

    const auto levelCount = std::max(get32(bytes, 40), 1U);
    if (bytes.size() < headerSize + (levelEntrySize * levelCount)) {
        SPONGE_ERROR("KTX2 level index is truncated");
        return {};
    }

    const auto kvdOffset = get32(bytes, kvdOffsetField);
    const auto kvdLength = get32(bytes, kvdLengthField);
    if (kvdOffset + kvdLength > bytes.size()) {
        SPONGE_ERROR("KTX2 key/value data runs past the end of the file");
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
            SPONGE_ERROR("KTX2 key/value entry has no key terminator");
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
            SPONGE_ERROR("KTX2 level {} runs past the end of the file", i);
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
