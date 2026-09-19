#pragma once

#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "event/mouseevent.hpp"
#include "layer/layer.hpp"
#include "ui/slider.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <tuple>

namespace game::layer {

enum class AudioMenuItem : uint8_t {
    MasterVolume = 0,
    SfxVolume,
    MusicVolume,
    Return,
    Count,
};

class AudioLayer final : public sponge::layer::Layer {
public:
    AudioLayer();

    void onAttach() override;

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    bool onUpdate(double elapsedTime) override;

private:
    AudioMenuItem                selectedItem = AudioMenuItem::MasterVolume;
    std::optional<AudioMenuItem> hoveredItem;

    // Row currently tracking the mouse; set on press, cleared on release.
    std::optional<AudioMenuItem> draggingItem;

    bool wasActiveLastFrame    = false;
    bool waitForConfirmRelease = false;

    std::unique_ptr<ui::Slider> masterVolumeSlider;
    std::unique_ptr<ui::Slider> sfxVolumeSlider;
    std::unique_ptr<ui::Slider> musicVolumeSlider;

    void renderRowBackground(float x, float y, float w, float h,
                             AudioMenuItem item) const;

    // Screen rect of a row, resolved through the root/menu/background chain.
    static std::tuple<float, float, float, float> rowLayout(AudioMenuItem item);

    // Widget backing a row, or nullptr for Return, which is a Button.
    ui::Slider* sliderFor(AudioMenuItem item) const;

    // Pushes a slider's current value to the audio engine and to Settings.
    // save chooses whether the write reaches disk now (a discrete step) or
    // waits for the drag to end (continuous mouse movement).
    void applyVolume(AudioMenuItem item, bool save) const;

    void close();

    static void recalculateLayout(float width, float height);

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event);

    bool onMouseButtonReleased(
        const sponge::event::MouseButtonReleasedEvent& event);

    bool onWindowResize(const sponge::event::WindowResizeEvent& event);

    void clearHoveredItems();
};

}  // namespace game::layer
