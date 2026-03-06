#pragma once

#include "sponge.hpp"

#include <optional>
#include <string>
#include <tuple>
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
    OptionMenuItem                selectedItem = OptionMenuItem::AspectRatio;
    std::optional<OptionMenuItem> hoveredItem;

    std::vector<sponge::core::Resolution> availableResolutions;
    std::vector<sponge::core::Resolution> filteredResolutions;
    size_t                                selectedAspectRatioIndex = 0;
    size_t                                selectedResolutionIndex  = 0;
    bool                                  hasUnappliedChanges      = false;

    void        renderRowBackground(float x, float y, float w, float h,
                                    OptionMenuItem item) const;
    static void renderRowText(float x, float y, float w, std::string_view label,
                              std::string_view value);
    static void renderCycleRow(float x, float y, float w, float h,
                               std::string_view label, std::string_view value,
                               bool hasLeft, bool hasRight, float maxValWidth);
    static void renderToggleRow(float x, float y, float w, float h,
                                std::string_view label, bool value);
    std::tuple<std::string, bool, bool> getResolutionDisplayInfo() const;

    float maxCycleValueWidth = 0.F;

    void filterResolutions();

    void recalculateLayout(float width, float height) const;

    void updateChangeStatus();

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event);

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event);

    bool onWindowResize(const sponge::event::WindowResizeEvent& event);

    void clearHoveredItems();

    void resetSelectionToCurrentState();
};

}  // namespace game::layer
