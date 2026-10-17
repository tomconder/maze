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

constexpr float volumeStep = 0.05F;

using game::layer::AudioMenuItem;

constexpr size_t rowCount = game::layer::audioRowCount;

constexpr std::array<const char*, rowCount> volumeKeys = {
    "audio.masterVolume",
    "audio.sfxVolume",
    "audio.musicVolume",
};

game::layer::ScrollMenuStyle audioStyle() {
    return {
        .name     = "audio",
        .tab      = game::ui::OptionTab::Audio,
        .rowCount = rowCount,
        .keyHints = game::layer::adjustKeyHints,
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
    };
    using sponge::core::Settings;
    for (size_t i = 0; i < rowCount; i++) {
        sliders[i] = std::make_unique<ui::Slider>(sliderCreateInfo);
        sliders[i]->setValue(
            static_cast<float>(Settings::getUInt32(volumeKeys[i], 100)) /
            100.F);
        sliders[i]->attach(rowNode(i));
    }
    recalculateLayout();
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

void AudioLayer::renderRow(const size_t row, const float y) {
    sliders[row]->onUpdate(y, volumeLabels[row]);
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
    const auto item   = static_cast<AudioMenuItem>(row);
    auto&      slider = *sliders[row];

    if (slider.isInsideTrack(mouse.x)) {
        slider.setValue(slider.valueAtX(mouse.x));
        applyVolume(item, false);
        return true;
    }
    if (const auto arrow = slider.arrowAt(mouse.x); arrow != 0) {
        if (slider.step(static_cast<float>(arrow) * volumeStep)) {
            applyVolume(item, true);
        }
    }
    return false;
}

void AudioLayer::onRowDrag(const size_t row, const glm::vec2& mouse) {
    auto& slider = *sliders[row];
    slider.setValue(slider.valueAtX(mouse.x));
    applyVolume(static_cast<AudioMenuItem>(row), false);
}

void AudioLayer::onRowReleased(const size_t row) {
    applyVolume(static_cast<AudioMenuItem>(row), true);
}

void AudioLayer::layoutRows(const ui::MenuLayout& layout,
                            const ui::RowText&    text) {
    layoutAudioRows(layout, text);
}

void AudioLayer::onFontSizeChanged() {
    for (const auto& slider : sliders) {
        slider->setFontSize(menuFontSize());
    }
}
}  // namespace game::layer
