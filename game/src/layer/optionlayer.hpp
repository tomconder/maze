#pragma once

#include "sponge.hpp"
#include "ui/checkbox.hpp"
#include "ui/selectlist.hpp"

#include <memory>
#include <optional>
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
    bool                                  hasUnappliedChanges = false;

    std::unique_ptr<ui::SelectList> aspectRatioList;
    std::unique_ptr<ui::SelectList> resolutionList;
    std::unique_ptr<ui::Checkbox>   fullScreenCheckbox;
    std::unique_ptr<ui::Checkbox>   verticalSyncCheckbox;

    void        renderRowBackground(float x, float y, float w, float h,
                                    OptionMenuItem item) const;
    static void renderRowText(float x, float y, float w, std::string_view label,
                              std::string_view value);

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
