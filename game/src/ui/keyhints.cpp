#include "ui/keyhints.hpp"

#include "input/activedevice.hpp"
#include "input/inputsnapshot.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/glfw/core/inputmanager.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/sprite.hpp"
#include "platform/opengl/scene/spriteatlas.hpp"
#include "ui/menufontsize.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace {
constexpr glm::vec3 labelColor = { 0.9F, 0.9F, 0.9F };

struct Metrics {
    uint32_t size;
    float    iconSize;
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

    // the prompt art is inset in its sprite, so it needs more room than the
    // label text to read at the same weight
    const auto iconSize = static_cast<float>(size) * 2.5F;

    return { size, iconSize, static_cast<float>(size) * 0.5F,
             windowWidth * 0.03F, iconSize * 0.3F };
}

// Single-key prompts fill 48 px of their 64 px sprite, the arrow clusters only
// 30 px, so single keys draw smaller to match the arrows.
float iconScale(const std::string_view icon) {
    return icon == "keyboard_enter" || icon == "keyboard_escape" ? 30.F / 48.F :
                                                                   1.F;
}

}  // namespace

namespace game::ui {
using sponge::platform::opengl::scene::BitmapFont;

const sponge::platform::opengl::scene::Sprite&
    promptSprite(const std::string_view name) {
    // The baked UI sprite sheet: the input prompt icons. Built on first use,
    // which needs a current GL context.
    static const sponge::platform::opengl::scene::SpriteAtlas atlas{
        "ui", "textures/ui.ktx2"
    };
    return atlas.sprite(name);
}

float keyHintBarHeight(const float windowWidth) {
    const auto metrics = metricsFor(windowWidth);
    return metrics.iconSize + metrics.marginY * 2.F;
}

float heightWithoutKeyHints(const float windowWidth, const float windowHeight) {
    return std::max(0.F, windowHeight - keyHintBarHeight(windowWidth));
}

void renderKeyHints(std::span<const KeyHint>           hints,
                    const std::shared_ptr<BitmapFont>& font,
                    const glm::mat4& projection, const float windowWidth,
                    const float windowHeight) {
    if (hints.empty()) {
        return;
    }

    const auto [size, iconSize, gap, marginX, marginY] =
        metricsFor(windowWidth);

    const auto iconTop = windowHeight - marginY - iconSize;
    const auto textTop =
        iconTop + (iconSize - static_cast<float>(font->getHeight(size))) / 2.F;

    const bool gamepad =
        sponge::platform::glfw::core::Application::get()
            .getInputManager()
            .getSnapshot()
            .activeDevice == sponge::input::ActiveDevice::Gamepad;

    // every prompt shares the one sprite shader
    const auto& shader = promptSprite(hints.front().keyIcon).getShader();
    shader->setMat4("projection", projection);

    const auto iconOf = [gamepad](const KeyHint& hint) {
        return gamepad ? hint.padIcon : hint.keyIcon;
    };
    const auto hintWidth = [&](const KeyHint& hint) {
        return iconSize * iconScale(iconOf(hint)) + gap +
               static_cast<float>(font->getLength(hint.label, size));
    };
    const auto renderHint = [&](const KeyHint& hint, const float x) {
        const auto icon      = iconOf(hint);
        const auto drawnSize = iconSize * iconScale(icon);

        promptSprite(icon).render(
            { x, iconTop + ((iconSize - drawnSize) / 2.F) },
            { drawnSize, drawnSize }, 1.F);

        font->beginPass(size);
        font->render(hint.label, { x + drawnSize + gap, textTop }, labelColor);
        font->endPass();
    };

    // Back sits at the left margin so it is in the same place on every menu.
    // Every other hint is right-aligned against the right margin, in order.
    const auto isBack = [](const KeyHint& hint) {
        return hint.keyIcon == "keyboard_escape";
    };
    auto leftEnd = marginX;
    for (const auto& hint : hints) {
        if (isBack(hint)) {
            renderHint(hint, marginX);
            leftEnd += hintWidth(hint);
        }
    }

    float rightWidth = 0.F;
    for (const auto& hint : hints) {
        if (!isBack(hint)) {
            rightWidth += hintWidth(hint) + gap * 2.F;
        }
    }
    rightWidth -= rightWidth > 0.F ? gap * 2.F : 0.F;

    // a window too narrow for both groups keeps Back whole and lets the rest
    // run past the right margin
    auto x = std::max(windowWidth - marginX - rightWidth, leftEnd + gap * 4.F);
    for (const auto& hint : hints) {
        if (!isBack(hint)) {
            renderHint(hint, x);
            x += hintWidth(hint) + gap * 2.F;
        }
    }
}
}  // namespace game::ui
