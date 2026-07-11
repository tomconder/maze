#pragma once

#include "sponge.hpp"
#include "ui/checkbox.hpp"
#include "ui/selectlist.hpp"

#include <memory>
#include <optional>

namespace game::layer {

enum class OptionMenuItem : uint8_t {
    AspectRatio = 0,
    Resolution,
    FullScreen,
    VerticalSync,
    AntiAliasing,
    ShadowQuality,
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

    bool hasUnappliedChanges   = false;
    bool wasActiveLastFrame    = false;
    bool waitForConfirmRelease = false;

    bool pendingFullscreen     = false;
    bool pendingVsync          = false;
    bool pendingFxaa           = false;
    int  pendingShadowResIndex = 1;

    // Mirrors the vsync value last requested via Maze::requestVerticalSync().
    // vsync is applied asynchronously on the render thread, so comparing
    // pendingVsync against the live Maze::hasVerticalSync() right after
    // applying would race the render thread; compare against this instead.
    bool appliedVsync = false;

    std::unique_ptr<ui::SelectList> aspectRatioList;
    std::unique_ptr<ui::SelectList> resolutionList;
    std::unique_ptr<ui::SelectList> shadowQualityList;
    std::unique_ptr<ui::Checkbox>   antiAliasingCheckbox;
    std::unique_ptr<ui::Checkbox>   fullScreenCheckbox;
    std::unique_ptr<ui::Checkbox>   verticalSyncCheckbox;

    void renderRowBackground(float x, float y, float w, float h,
                             OptionMenuItem item) const;

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
