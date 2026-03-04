#pragma once

#include "sponge.hpp"

#include <vector>

namespace game::layer {

enum class OptionMenuItem : uint8_t {
    AspectRatio = 0,
    Resolution,
    FullScreen,
    VerticalSync,
    Return,
    Count
};

class OptionLayer final : public sponge::layer::Layer {
public:
    OptionLayer();

    void onAttach() override;

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    bool onUpdate(double elapsedTime) override;

private:
    OptionMenuItem selectedItem = OptionMenuItem::AspectRatio;

    std::vector<sponge::core::Resolution> availableResolutions;
    std::vector<sponge::core::Resolution> filteredResolutions;
    size_t                                selectedAspectRatioIndex = 0;
    size_t                                selectedResolutionIndex  = 0;
    bool                                  hasUnappliedChanges      = false;

    void filterResolutions();

    void recalculateLayout(float width, float height) const;

    void updateChangeStatus();

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event);

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event) const;

    bool onWindowResize(const sponge::event::WindowResizeEvent& event);

    void clearHoveredItems() const;
};

}  // namespace game::layer
