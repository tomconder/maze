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

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace {
constexpr glm::vec3 labelColor = { 0.9F, 0.9F, 0.9F };
constexpr glm::vec3 hoverColor = { 1.F, 0.45F, 0.45F };

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
    return icon == "keyboard_enter" || icon == "keyboard_escape" ||
                   icon == "keyboard_r" ?
               30.F / 48.F :
               1.F;
}

std::string_view iconOf(const game::ui::KeyHint& hint, const bool gamepad) {
    return gamepad ? hint.padIcon : hint.keyIcon;
}

bool isBack(const game::ui::KeyHint& hint) {
    return hint.keyIcon == "keyboard_escape";
}

// Back and the hints marked leftAligned sit at the left margin.
bool isLeft(const game::ui::KeyHint& hint) {
    return isBack(hint) || hint.leftAligned;
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

float keyHintMarginX(const float windowWidth) {
    return metricsFor(windowWidth).marginX;
}

float keyHintGap(const float windowWidth) {
    return metricsFor(windowWidth).gap;
}

bool usingGamepad() {
    return sponge::platform::glfw::core::Application::get()
               .getInputManager()
               .getSnapshot()
               .activeDevice == sponge::input::ActiveDevice::Gamepad;
}

TextMeasure fontMeasure(const std::shared_ptr<BitmapFont>& font) {
    return [font](const std::string_view text, const uint32_t size) {
        return TextSize{ static_cast<float>(font->getLength(text, size)),
                         static_cast<float>(font->getHeight(size)) };
    };
}

KeyHintOrder keyHintOrder(const std::span<const KeyHint> hints) {
    KeyHintOrder order;
    for (const bool backPass : { true, false }) {
        for (size_t i = 0; i < hints.size(); i++) {
            if (isLeft(hints[i]) && isBack(hints[i]) == backPass) {
                order.left.push_back(i);
            }
        }
    }
    for (size_t i = 0; i < hints.size(); i++) {
        if (!isLeft(hints[i])) {
            order.right.push_back(i);
        }
    }
    return order;
}

std::vector<KeyHintSize> keyHintSizes(const std::span<const KeyHint> hints,
                                      const float        windowWidth,
                                      const bool         gamepad,
                                      const TextMeasure& measure) {
    const auto metrics = metricsFor(windowWidth);

    std::vector<KeyHintSize> sizes;
    for (const auto& hint : hints) {
        sizes.push_back({ metrics.iconSize * iconScale(iconOf(hint, gamepad)),
                          measure(hint.label, metrics.size) });
    }
    return sizes;
}

void renderKeyHints(const std::span<const KeyHint>     hints,
                    const std::shared_ptr<BitmapFont>& font,
                    const glm::mat4& projection, const float windowWidth,
                    const std::span<const KeyHintRects> rects,
                    const std::optional<size_t>         hovered) {
    if (hints.empty()) {
        return;
    }

    const auto size    = metricsFor(windowWidth).size;
    const bool gamepad = usingGamepad();

    // every prompt shares the one sprite shader
    const auto& shader = promptSprite(hints.front().keyIcon).getShader();
    shader->setMat4("projection", projection);

    for (size_t i = 0; i < hints.size(); i++) {
        const auto& [hint, icon, label] = rects[i];

        promptSprite(iconOf(hints[i], gamepad))
            .render({ icon.x, icon.y }, { icon.width, icon.height }, 1.F);

        font->beginPass(size);
        font->render(hints[i].label, { label.x, label.y },
                     hovered == i ? hoverColor : labelColor);
        font->endPass();
    }
}

std::optional<size_t> hitTestKeyHints(const std::span<const KeyHintRects> rects,
                                      const glm::vec2& position) {
    for (size_t i = 0; i < rects.size(); i++) {
        const auto& hint = rects[i].hint;
        if (position.x >= hint.x && position.x <= hint.x + hint.width &&
            position.y >= hint.y && position.y <= hint.y + hint.height) {
            return i;
        }
    }
    return std::nullopt;
}
}  // namespace game::ui
