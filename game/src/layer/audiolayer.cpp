#include "layer/audiolayer.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

#include "core/settings.hpp"
#include "input/gameaction.hpp"
#include "platform/audio/audio.hpp"
#include "ui/keyhints.hpp"
#include "ui/slider.hpp"

namespace {
constexpr glm::vec3 textColor          = { 1.F, 1.F, 1.F };
constexpr glm::vec3 arrowDisabledColor = { 0.4F, 0.4F, 0.4F };

constexpr float textMarginLeft = 26.F;
constexpr float volumeStep     = 0.05F;

using game::layer::AudioMenuItem;

constexpr size_t rowCount = static_cast<size_t>(AudioMenuItem::Count);

// Rows follow AudioMenuItem order, so the enum is the row index.
constexpr std::array<std::string_view, rowCount> volumeLabels = {
    "Master Volume",
    "Sound Effects",
    "Music",
};

constexpr std::array<const char*, rowCount> volumeKeys = {
    "audio.masterVolume",
    "audio.sfxVolume",
    "audio.musicVolume",
};

constexpr std::array<game::ui::KeyHint, 4> audioKeyHints = {
    game::ui::KeyHint{ "keyboard_arrows_vertical", "xbox_dpad_vertical",
                       "Navigate" },
    game::ui::KeyHint{ "keyboard_arrows_horizontal", "xbox_dpad_horizontal",
                       "Change" },
    game::ui::KeyHint{ "keyboard_enter", "xbox_button_a", "Select" },
    game::ui::KeyHint{ "keyboard_escape", "xbox_button_b", "Back" },
};

game::layer::ScrollMenuStyle audioStyle() {
    return {
        .name     = "audio",
        .tab      = game::ui::OptionTab::Audio,
        .rowCount = rowCount,
        .keyHints = audioKeyHints,
    };
}
}  // namespace

namespace game::layer {

AudioLayer::AudioLayer() : ScrollMenuLayer(audioStyle()) {}

void AudioLayer::onAttach() {
    ScrollMenuLayer::onAttach();

    const ui::SliderCreateInfo sliderCreateInfo{
        .font               = menuFont(),
        .fontSize           = menuFontSize(),
        .textColor          = textColor,
        .arrowDisabledColor = arrowDisabledColor,
        .textMarginLeft     = textMarginLeft,
    };
    using sponge::core::Settings;
    for (size_t i = 0; i < rowCount; i++) {
        sliders[i] = std::make_unique<ui::Slider>(sliderCreateInfo);
        sliders[i]->setValue(
            static_cast<float>(Settings::getUInt32(volumeKeys[i], 100)) /
            100.F);
    }
}

void AudioLayer::applyVolume(const AudioMenuItem item, const bool save) const {
    using sponge::core::Settings;
    using sponge::platform::audio::Audio;

    const auto value   = sliders[static_cast<size_t>(item)]->getValue();
    const auto percent = static_cast<uint32_t>(std::lround(value * 100.F));

    switch (item) {
        case AudioMenuItem::MasterVolume:
            Audio::setMasterVolume(value);
            Settings::set("audio.masterVolume", percent);
            break;
        case AudioMenuItem::SfxVolume:
            Audio::setSfxVolume(value);
            Settings::set("audio.sfxVolume", percent);
            break;
        case AudioMenuItem::MusicVolume:
            Audio::setMusicVolume(value);
            Settings::set("audio.musicVolume", percent);
            break;
        default:
            return;
    }

    if (save) {
        Settings::save();
    }
}

void AudioLayer::renderRow(const size_t row, const float x, const float y,
                           const float w, const float h) {
    sliders[row]->onUpdate(x, y, w, h, volumeLabels[row]);
}

void AudioLayer::onRowKeys(const size_t                        row,
                           const sponge::input::InputSnapshot& input) {
    using sponge::input::GameAction;
    const auto item = static_cast<AudioMenuItem>(row);

    auto& slider = *sliders[row];

    bool changed = false;
    if (input.isActive(GameAction::MenuLeft)) {
        changed = slider.step(-volumeStep);
    }
    if (input.isActive(GameAction::MenuRight)) {
        changed = slider.step(volumeStep) || changed;
    }
    if (changed) {
        applyVolume(item, true);
    }
}

bool AudioLayer::onRowPressed(const size_t row, const glm::vec2& mouse) {
    const auto [x, y, w, h] = rowRect(row);
    const auto item         = static_cast<AudioMenuItem>(row);
    auto&      slider       = *sliders[row];

    if (slider.isInsideTrack(mouse.x, x, w)) {
        slider.setValue(slider.valueAtX(mouse.x, x, w));
        applyVolume(item, false);
        return true;
    }
    if (const auto arrow = slider.arrowAt(mouse.x, x, w); arrow != 0) {
        if (slider.step(static_cast<float>(arrow) * volumeStep)) {
            applyVolume(item, true);
        }
    }
    return false;
}

void AudioLayer::onRowDrag(const size_t row, const glm::vec2& mouse) {
    const auto [x, y, w, h] = rowRect(row);
    auto& slider            = *sliders[row];
    slider.setValue(slider.valueAtX(mouse.x, x, w));
    applyVolume(static_cast<AudioMenuItem>(row), false);
}

void AudioLayer::onRowReleased(const size_t row) {
    applyVolume(static_cast<AudioMenuItem>(row), true);
}

void AudioLayer::onFontSizeChanged() {
    for (const auto& slider : sliders) {
        slider->setFontSize(menuFontSize());
    }
}
}  // namespace game::layer
