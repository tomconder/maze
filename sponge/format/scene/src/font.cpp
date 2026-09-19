#include "font.hpp"

#include <fmt/format.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
using sponge::scene::font::Font;
using sponge::scene::font::Glyph;
using sponge::scene::font::Size;
using sponge::scene::font::subpixelPhases;
using sponge::scene::font::version;

struct Header {
    uint32_t version;
    uint32_t slotCount;
    uint32_t slotMapCount;
    uint32_t tabularMapCount;
    uint32_t sizeCount;
};

struct SlotEntry {
    uint32_t codepoint;
    uint32_t slot;
};

struct SizeHeader {
    uint32_t pixels;
    float    lineHeight;
    float    ascender;
    uint32_t kerningCount;
};

struct KerningEntry {
    uint32_t pair;
    int32_t  adjustment;
};

static_assert(sizeof(Glyph) == 32);

template <typename T>
void append(std::vector<uint8_t>& out, const T* items, const size_t count) {
    const auto offset = out.size();
    out.resize(offset + (sizeof(T) * count));
    if (count != 0) {
        std::memcpy(out.data() + offset, items, sizeof(T) * count);
    }
}

template <typename T>
void append(std::vector<uint8_t>& out, const T& item) {
    append(out, &item, 1);
}

void appendSlots(std::vector<uint8_t>&               out,
                 const std::map<char32_t, uint32_t>& slots) {
    for (const auto& [codepoint, slot] : slots) {
        append(out, SlotEntry{ .codepoint = codepoint, .slot = slot });
    }
}

// Reads trivially copyable records in order, refusing to run past the end.
class Reader {
public:
    explicit Reader(const std::span<const uint8_t> bytes) : bytes(bytes) {}

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    bool take(T* items, const size_t count) {
        if (count > (bytes.size() - at) / sizeof(T)) {
            return false;
        }
        if (count != 0) {
            std::memcpy(items, bytes.data() + at, sizeof(T) * count);
        }
        at += sizeof(T) * count;
        return true;
    }

    bool done() const {
        return at == bytes.size();
    }

private:
    std::span<const uint8_t> bytes;
    size_t                   at = 0;
};

bool takeSlots(Reader& reader, const uint32_t count, const uint32_t slotCount,
               std::map<char32_t, uint32_t>& slots) {
    for (uint32_t i = 0; i < count; i++) {
        SlotEntry entry{};
        if (!reader.take(&entry, 1) || entry.slot >= slotCount) {
            return false;
        }
        slots.emplace(entry.codepoint, entry.slot);
    }
    return true;
}

// Invalid or truncated sequences decode to U+FFFD, one byte at a time.
char32_t nextCodepoint(const std::string_view text, size_t& at) {
    constexpr char32_t replacement = 0xFFFD;

    const auto lead      = static_cast<uint8_t>(text[at]);
    size_t     length    = 0;
    char32_t   codepoint = 0;
    if (lead < 0x80) {
        at++;
        return lead;
    }
    if ((lead & 0xE0) == 0xC0) {
        length    = 2;
        codepoint = lead & 0x1FU;
    } else if ((lead & 0xF0) == 0xE0) {
        length    = 3;
        codepoint = lead & 0x0FU;
    } else if ((lead & 0xF8) == 0xF0) {
        length    = 4;
        codepoint = lead & 0x07U;
    } else {
        at++;
        return replacement;
    }

    if (at + length > text.size()) {
        at++;
        return replacement;
    }
    for (size_t i = 1; i < length; i++) {
        const auto next = static_cast<uint8_t>(text[at + i]);
        if ((next & 0xC0) != 0x80) {
            at++;
            return replacement;
        }
        codepoint = (codepoint << 6) | (next & 0x3FU);
    }
    at += length;
    return codepoint;
}
}  // namespace

namespace sponge::scene::font {

std::vector<uint8_t> write(const Font& font) {
    std::vector<uint8_t> out;
    append(out, Header{
                    .version      = version,
                    .slotCount    = font.slotCount,
                    .slotMapCount = static_cast<uint32_t>(font.slots.size()),
                    .tabularMapCount =
                        static_cast<uint32_t>(font.tabularSlots.size()),
                    .sizeCount = static_cast<uint32_t>(font.sizes.size()),
                });
    appendSlots(out, font.slots);
    appendSlots(out, font.tabularSlots);

    for (const auto& size : font.sizes) {
        append(out,
               SizeHeader{
                   .pixels       = size.pixels,
                   .lineHeight   = size.lineHeight,
                   .ascender     = size.ascender,
                   .kerningCount = static_cast<uint32_t>(size.kerning.size()),
               });
        append(out, size.advances.data(), size.advances.size());
        append(out, size.glyphs.data(), size.glyphs.size());
        for (const auto& [pair, adjustment] : size.kerning) {
            append(out, KerningEntry{ .pair = pair, .adjustment = adjustment });
        }
    }
    return out;
}

Font read(const std::span<const uint8_t> bytes, std::string& error) {
    Reader reader{ bytes };
    Header header{};
    if (!reader.take(&header, 1)) {
        error = "Font tables are truncated";
        return {};
    }
    if (header.version != version) {
        error = fmt::format("Font tables version {}, expected {}",
                            header.version, version);
        return {};
    }
    // Slot 0 must exist. The byte bound keeps a corrupt count from sizing
    // the vectors below.
    if (header.slotCount == 0 || header.slotCount > bytes.size() ||
        header.sizeCount > bytes.size()) {
        error = fmt::format("Font tables claim {} slots and {} sizes",
                            header.slotCount, header.sizeCount);
        return {};
    }

    Font font;
    font.slotCount = header.slotCount;
    if (!takeSlots(reader, header.slotMapCount, header.slotCount, font.slots) ||
        !takeSlots(reader, header.tabularMapCount, header.slotCount,
                   font.tabularSlots)) {
        error = "Font codepoint tables are truncated or out of range";
        return {};
    }

    std::vector<Size> sizes(header.sizeCount);
    for (auto& size : sizes) {
        SizeHeader sizeHeader{};
        size.advances.resize(header.slotCount);
        size.glyphs.resize(static_cast<size_t>(header.slotCount) *
                           subpixelPhases);
        if (!reader.take(&sizeHeader, 1) ||
            !reader.take(size.advances.data(), size.advances.size()) ||
            !reader.take(size.glyphs.data(), size.glyphs.size())) {
            error = "Font size tables are truncated";
            return {};
        }
        size.pixels     = sizeHeader.pixels;
        size.lineHeight = sizeHeader.lineHeight;
        size.ascender   = sizeHeader.ascender;
        for (uint32_t i = 0; i < sizeHeader.kerningCount; i++) {
            KerningEntry entry{};
            if (!reader.take(&entry, 1)) {
                error = "Font kerning table is truncated";
                return {};
            }
            size.kerning.emplace(entry.pair, entry.adjustment);
        }
    }
    if (!reader.done()) {
        error = "Font tables have trailing bytes";
        return {};
    }

    font.sizes = std::move(sizes);
    return font;
}

const Size* sizeOf(const Font& font, const uint32_t pixels) {
    for (const auto& size : font.sizes) {
        if (size.pixels == pixels) {
            return &size;
        }
    }
    return nullptr;
}

void shape(const Font& font, const Size& size, const std::string_view text,
           const bool tabularFigures, std::vector<ShapedGlyph>& glyphs) {
    const auto& slots = tabularFigures ? font.tabularSlots : font.slots;

    glyphs.clear();
    for (size_t at = 0; at < text.size();) {
        const auto found = slots.find(nextCodepoint(text, at));
        const auto slot  = found != slots.end() ? found->second : 0;

        if (!glyphs.empty()) {
            const auto kerning =
                size.kerning.find((glyphs.back().slot << 16) | slot);
            if (kerning != size.kerning.end()) {
                glyphs.back().xAdvance +=
                    static_cast<float>(kerning->second) / 64.F;
            }
        }
        glyphs.push_back(
            { .slot     = slot,
              .xAdvance = static_cast<float>(size.advances[slot]) / 64.F });
    }
}

}  // namespace sponge::scene::font
