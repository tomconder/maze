#pragma once

#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "event/mouseevent.hpp"
#include "layer/layer.hpp"
#include "ui/checkbox.hpp"
#include "ui/confirmdialog.hpp"
#include "ui/selectlist.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <tuple>

namespace game::layer {

enum class OptionMenuItem : uint8_t {
    GeneralHeading = 0,
    DisplayMode,
    AspectRatio,
    Resolution,
    VerticalSync,
    AntiAliasing,
    QualityHeading,
    ShadowQuality,
    AdvancedHeading,
    AmbientOcclusion,
    Bloom,
    SubsurfaceScattering,
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
    OptionMenuItem                selectedItem = OptionMenuItem::DisplayMode;
    std::optional<OptionMenuItem> hoveredItem;

    bool wasActiveLastFrame    = false;
    bool waitForConfirmRelease = false;

    // Every row applies as soon as it changes. A display mode change opens
    // the confirm layer and is undone unless it is accepted.
    bool pendingVsync                = false;
    bool pendingAmbientOcclusion     = true;
    bool pendingBloom                = true;
    bool pendingSubsurfaceScattering = true;
    int  pendingDisplayModeIndex     = 0;
    int  pendingAaIndex              = 0;
    int  pendingShadowQualityIndex   = 1;

    std::unique_ptr<ui::SelectList> aspectRatioList;
    std::unique_ptr<ui::SelectList> resolutionList;
    std::unique_ptr<ui::SelectList> displayModeList;
    std::unique_ptr<ui::SelectList> shadowQualityList;
    std::unique_ptr<ui::SelectList> antiAliasingList;
    std::unique_ptr<ui::Checkbox>   verticalSyncCheckbox;
    std::unique_ptr<ui::Checkbox>   ambientOcclusionCheckbox;
    std::unique_ptr<ui::Checkbox>   bloomCheckbox;
    std::unique_ptr<ui::Checkbox>   subsurfaceScatteringCheckbox;

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

    // Lays out again at the current window size, after the scroll moved.
    static void relayout();

    // Scrolls the selected row into view.
    void followSelection();

    // The mode asks to be kept; the resolution applies and saves at once.
    void applyDisplayMode();
    void applyResolution();

    // Keeps or undoes the mode change the dialog asked about. No-op while
    // Pending.
    void resolveDisplayChange(ui::ConfirmResult result);

    void syncPendingCheckboxState();

    void updateChangeStatus();

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event);

    bool onMouseButtonReleased(
        const sponge::event::MouseButtonReleasedEvent& event);

    bool onMouseScrolled(const sponge::event::MouseScrolledEvent& event);

    bool onWindowResize(const sponge::event::WindowResizeEvent& event);

    void clearHoveredItems();

    void resetSelectionToCurrentState();
};

}  // namespace game::layer
