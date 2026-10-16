#include "ui/fontmetrics.hpp"

#include "ktx2.hpp"
#include "readbytes.hpp"

#include <algorithm>
#include <cmath>

namespace game::ui {

FontMetrics::FontMetrics(const std::string& path) {
    const auto  bytes = sponge::scene::readBytes(path);
    std::string error;
    const auto  image = sponge::scene::ktx2::read(bytes, error);
    const auto  tables =
        std::ranges::find_if(image.keyValues, [](const auto& entry) {
            return entry.first == sponge::scene::font::fontKey;
        });
    if (tables != image.keyValues.end()) {
        font = sponge::scene::font::read(tables->second, error);
    }
}

uint32_t FontMetrics::length(const std::string_view text, const uint32_t size) {
    const auto* baked = sponge::scene::font::sizeOf(font, size);
    if (baked == nullptr) {
        return 0;
    }

    sponge::scene::font::shape(font, *baked, text, false, shaped);
    float penX = 0.F;
    for (const auto& glyph : shaped) {
        penX += glyph.xAdvance;
    }
    return static_cast<uint32_t>(std::lround(penX));
}

uint32_t FontMetrics::height(const uint32_t size) const {
    const auto* baked = sponge::scene::font::sizeOf(font, size);
    return baked != nullptr ? static_cast<uint32_t>(baked->lineHeight) : size;
}

}  // namespace game::ui
