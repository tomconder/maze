#include "ui/keyhints.hpp"

#include "input/activedevice.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/quad.hpp"
#include "ui/menufontsize.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <span>

namespace {
constexpr glm::vec4 capColor     = { 0.85F, 0.85F, 0.85F, 0.92F };
constexpr glm::vec3 capTextColor = { 0.05F, 0.05F, 0.05F };
constexpr glm::vec3 labelColor   = { 0.9F, 0.9F, 0.9F };

struct Metrics {
    uint32_t size;
    float    capHeight;
    float    gap;
    float    marginX;
    float    marginY;
};

Metrics metricsFor(const float windowWidth) {
    // one step below the menu size, and only sizes the atlas bakes
    // (18, 24, 32, 48) — an unbaked size renders no glyphs
    const auto menuSize =
        game::ui::menuFontSizeForWidth(static_cast<uint32_t>(windowWidth));
    const uint32_t size = menuSize <= 24 ? 18 : menuSize <= 32 ? 24 : 32;

    const auto capHeight = static_cast<float>(size) * 1.5F;

    return { size, capHeight, static_cast<float>(size) * 0.5F,
             windowWidth * 0.03F, capHeight * 0.5F };
}
}  // namespace

namespace game::ui {
using sponge::platform::opengl::scene::BitmapFont;
using sponge::platform::opengl::scene::Quad;

float keyHintBarHeight(const float windowWidth) {
    const auto metrics = metricsFor(windowWidth);
    return metrics.capHeight + metrics.marginY * 2.F;
}

void renderKeyHints(std::span<const KeyHint>           hints,
                    const std::shared_ptr<BitmapFont>& font,
                    const float windowWidth, const float windowHeight) {
    static const Quad quad;

    const auto [size, capHeight, gap, marginX, marginY] =
        metricsFor(windowWidth);

    const auto capTop  = windowHeight - marginY - capHeight;
    const auto textTop = capTop + (capHeight - static_cast<float>(size)) / 2.F;

    const bool gamepad =
        sponge::platform::glfw::core::Application::get()
            .getInputManager()
            .getSnapshot()
            .activeDevice == sponge::input::ActiveDevice::Gamepad;

    auto x = marginX;
    for (const auto& hint : hints) {
        const auto cap      = gamepad ? hint.pad : hint.key;
        const auto keyWidth = static_cast<float>(font->getLength(cap, size));
        const auto capWidth = std::max(capHeight, keyWidth + capHeight * 0.6F);

        quad.render({ x, capTop }, { x + capWidth, capTop + capHeight },
                    capColor, capHeight * 0.2F);

        font->beginPass(size);
        font->render(cap, { x + (capWidth - keyWidth) / 2.F, textTop },
                     capTextColor);
        font->render(hint.label, { x + capWidth + gap, textTop }, labelColor);
        font->endPass();

        x += capWidth + gap +
             static_cast<float>(font->getLength(hint.label, size)) + gap * 2.F;
    }
}
}  // namespace game::ui
