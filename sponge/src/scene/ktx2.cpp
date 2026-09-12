#include "scene/ktx2.hpp"

#include "logging/log.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace {
constexpr uint8_t identifier[12] = { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32,
                                     0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };

// Fixed part of a KTX2 file: 12-byte identifier, 9 u32 header fields, then
// the index (4 u32 + 2 u64). The level index follows at this offset.
constexpr size_t headerSize     = 80;
constexpr size_t levelEntrySize = 24;

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

// typeSize is the size of one channel for uncompressed formats and 1 for
// block-compressed ones.
uint32_t typeSizeFor(const sponge::scene::ktx2::Format format) {
    switch (format) {
        case sponge::scene::ktx2::formatBc5Unorm:
        case sponge::scene::ktx2::formatBc7Unorm:
        case sponge::scene::ktx2::formatBc7Srgb:
            return 1;
        default:
            return 1;
    }
}
}  // namespace

namespace sponge::scene::ktx2 {

std::vector<uint8_t> write(const Format format, const uint32_t width,
                           const uint32_t                              height,
                           const std::span<const std::vector<uint8_t>> levels) {
    const auto levelCount = levels.size();

    // Level data is written largest first, immediately after the level
    // index. Offsets are absolute from the start of the file.
    std::vector<uint64_t> offsets(levelCount);
    auto                  cursor = headerSize + (levelEntrySize * levelCount);
    for (size_t i = 0; i < levelCount; i++) {
        offsets[i] = cursor;
        cursor += levels[i].size();
    }

    std::vector<uint8_t> out;
    out.reserve(cursor);
    out.insert(out.end(), std::begin(identifier), std::end(identifier));

    put32(out, format);
    put32(out, typeSizeFor(format));
    put32(out, width);
    put32(out, height);
    put32(out, 0);  // pixelDepth
    put32(out, 0);  // layerCount
    put32(out, 1);  // faceCount
    put32(out, static_cast<uint32_t>(levelCount));
    put32(out, 0);  // supercompressionScheme: none

    put32(out, 0);  // dfdByteOffset
    put32(out, 0);  // dfdByteLength
    put32(out, 0);  // kvdByteOffset
    put32(out, 0);  // kvdByteLength
    put64(out, 0);  // sgdByteOffset
    put64(out, 0);  // sgdByteLength

    for (size_t i = 0; i < levelCount; i++) {
        put64(out, offsets[i]);
        put64(out, levels[i].size());
        put64(out, levels[i].size());  // uncompressedByteLength
    }

    for (const auto& level : levels) {
        out.insert(out.end(), level.begin(), level.end());
    }

    return out;
}

std::vector<uint8_t> write(const Format format, const uint32_t width,
                           const uint32_t                 height,
                           const std::span<const uint8_t> pixels) {
    const std::vector<std::vector<uint8_t>> levels{ std::vector<uint8_t>{
        pixels.begin(), pixels.end() } };
    return write(format, width, height, levels);
}

Image read(const std::span<const uint8_t> bytes) {
    if (bytes.size() < headerSize ||
        !std::equal(std::begin(identifier), std::end(identifier),
                    bytes.begin())) {
        SPONGE_ERROR("Not a KTX2 file");
        return {};
    }

    const auto supercompression = get32(bytes, 48);
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
