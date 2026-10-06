#include "atlas.hpp"

#include "ktx2.hpp"
#include "modeldata.hpp"
#include "rectpack.hpp"
#include "texenc.hpp"

#include <fmt/base.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace {
namespace ktx2 = sponge::scene::ktx2;
using sponge::scene::ParsedImage;

constexpr uint32_t channels = 4;

// Each sprite is surrounded by a one-texel border holding a copy of its edge
// pixels. A linear sample at the very edge of a sprite then reads that copy
// instead of the neighbouring sprite.
constexpr uint32_t gutter = 1;

// The atlas is as small as the packer can make it, up to this side. If the
// sprites do not fit, the atlas is being asked to hold something that should
// be its own texture.
constexpr int maxSide = 2048;

// Copies a sprite in at (x, y) and extends its edge pixels into the gutter.
void blit(std::vector<uint8_t>& atlas, const uint32_t atlasWidth,
          const ParsedImage& source, const uint32_t x, const uint32_t y) {
    for (int32_t row = -static_cast<int32_t>(gutter);
         row < static_cast<int32_t>(source.height + gutter); row++) {
        const auto sourceRow =
            std::clamp(row, 0, static_cast<int32_t>(source.height) - 1);
        for (int32_t col = -static_cast<int32_t>(gutter);
             col < static_cast<int32_t>(source.width + gutter); col++) {
            const auto sourceCol =
                std::clamp(col, 0, static_cast<int32_t>(source.width) - 1);
            const auto* src =
                &source
                     .pixels[((static_cast<size_t>(sourceRow) * source.width) +
                              sourceCol) *
                             channels];
            auto* dst = &atlas[(((static_cast<size_t>(y) + row) * atlasWidth) +
                                x + col) *
                               channels];
            std::copy_n(src, channels, dst);
        }
    }
}

// "name x y w h" per line. Text because it is inspectable in a hex dump of
// the container, and the table is fourteen lines long.
std::vector<uint8_t>
    rectTable(const std::vector<ParsedImage>&           sources,
              const std::vector<rectpack2D::rect_xywh>& rects) {
    std::string table;
    for (size_t i = 0; i < sources.size(); i++) {
        table += sources[i].name + " " + std::to_string(rects[i].x + gutter) +
                 " " + std::to_string(rects[i].y + gutter) + " " +
                 std::to_string(sources[i].width) + " " +
                 std::to_string(sources[i].height) + "\n";
    }
    return { table.begin(), table.end() };
}
}  // namespace

namespace assetconv {

std::vector<uint8_t> packAtlas(const std::vector<AtlasEntry>& entries) {
    std::vector<ParsedImage> sources;
    for (const auto& entry : entries) {
        sources.push_back(loadImage(entry.path));
        if (sources.back().width == 0) {
            return {};
        }
        sources.back().name = entry.name;
    }

    std::vector<rectpack2D::rect_xywh> rects(sources.size());
    for (size_t i = 0; i < sources.size(); i++) {
        rects[i].w = static_cast<int>(sources[i].width + (gutter * 2));
        rects[i].h = static_cast<int>(sources[i].height + (gutter * 2));
    }

    const auto packed = packRects(rects, maxSide);
    if (packed.w == 0) {
        fmt::println(stderr,
                     "assetconv: {} sprites do not fit in a {}x{} atlas",
                     entries.size(), maxSide, maxSide);
        return {};
    }
    const auto width  = static_cast<uint32_t>(packed.w);
    const auto height = static_cast<uint32_t>(packed.h);

    std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * channels);
    for (size_t i = 0; i < sources.size(); i++) {
        blit(pixels, width, sources[i],
             static_cast<uint32_t>(rects[i].x) + gutter,
             static_cast<uint32_t>(rects[i].y) + gutter);
    }

    // One level, and UNORM rather than SRGB: this is what the engine
    // uploaded for these images before they were baked. Mips would bleed
    // between sprites anyway, and UI is drawn at native size.
    const std::vector<ktx2::KeyValue> keyValues{
        { "spongeAtlas", rectTable(sources, rects) }
    };
    return ktx2::write(ktx2::formatR8G8B8A8Unorm, width, height,
                       std::vector<std::vector<uint8_t>>{ std::move(pixels) },
                       keyValues);
}

}  // namespace assetconv
