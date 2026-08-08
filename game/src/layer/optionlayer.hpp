#pragma once

#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "event/mouseevent.hpp"
#include "layer/layer.hpp"
#include "ui/checkbox.hpp"
#include "ui/selectlist.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <tuple>

namespace game::layer {

enum class OptionMenuItem : uint8_t {
    AspectRatio = 0,
    Resolution,
    FullScreen,
    VerticalSync,
    AntiAliasing,
    ShadowQuality,
    Return,
    Count,
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

    bool hasUnappliedChanges   = false;
    bool wasActiveLastFrame    = false;
    bool waitForConfirmRelease = false;

    bool pendingFullscreen     = false;
    bool pendingVsync          = false;
    bool pendingFxaa           = false;
    int  pendingShadowResIndex = 1;

    std::unique_ptr<ui::SelectList> aspectRatioList;
    std::unique_ptr<ui::SelectList> resolutionList;
    std::unique_ptr<ui::SelectList> shadowQualityList;
    std::unique_ptr<ui::Checkbox>   antiAliasingCheckbox;
    std::unique_ptr<ui::Checkbox>   fullScreenCheckbox;
    std::unique_ptr<ui::Checkbox>   verticalSyncCheckbox;

    void renderRowBackground(float x, float y, float w, float h,
                             OptionMenuItem item) const;

    // Screen rect of a row, resolved through the root/menu/background chain.
    static std::tuple<float, float, float, float>
        rowLayout(OptionMenuItem item);

    // Widget backing a row, or nullptr when the row is not of that kind.
    ui::SelectList* listFor(OptionMenuItem item) const;
    ui::Checkbox*   checkboxFor(OptionMenuItem item) const;
    bool*           pendingFor(OptionMenuItem item);

    void cycleList(OptionMenuItem item, int delta);

    void togglePending(OptionMenuItem item);

    void filterResolutions();

    static void recalculateLayout(float width, float height);

    void applyChanges();

    void syncPendingCheckboxState();

    void updateChangeStatus();

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event);

    bool onWindowResize(const sponge::event::WindowResizeEvent& event);

    void clearHoveredItems();

    void resetSelectionToCurrentState();
};

}  // namespace game::layer
