#include "ui/keyhints.hpp"

#include "input/activedevice.hpp"
#include "input/inputsnapshot.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/glfw/core/inputmanager.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/sprite.hpp"
#include "ui/menufontsize.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

namespace {
constexpr glm::vec3 labelColor     = { 0.9F, 0.9F, 0.9F };
constexpr glm::vec3 rightTextColor = { 0.6F, 0.6F, 0.6F };

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

// Prompt sprites, built on first use — construction needs a GL context.
const sponge::platform::opengl::scene::Sprite&
    promptSprite(const std::string_view name) {
    static std::unordered_map<
        std::string, std::unique_ptr<sponge::platform::opengl::scene::Sprite>>
        sprites;

    auto& sprite = sprites[std::string(name)];
    if (!sprite) {
        sprite = std::make_unique<sponge::platform::opengl::scene::Sprite>(
            std::string(name),
            "textures/prompts/" + std::string(name) + ".png");
    }
    return *sprite;
}
}  // namespace

namespace game::ui {
using sponge::platform::opengl::scene::BitmapFont;

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
                    const float            windowHeight,
                    const std::string_view rightText) {
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
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    auto x = marginX;
    for (const auto& hint : hints) {
        const auto icon = gamepad ? hint.padIcon : hint.keyIcon;

        promptSprite(icon).render({ x, iconTop }, { iconSize, iconSize }, 1.F);

        font->beginPass(size);
        font->render(hint.label, { x + iconSize + gap, textTop }, labelColor);
        font->endPass();

        x += iconSize + gap +
             static_cast<float>(font->getLength(hint.label, size)) + gap * 2.F;
    }

    if (!rightText.empty()) {
        const auto textWidth =
            static_cast<float>(font->getLength(rightText, size));
        font->beginPass(size);
        font->render(rightText, { windowWidth - marginX - textWidth, textTop },
                     rightTextColor);
        font->endPass();
    }
}
}  // namespace game::ui
