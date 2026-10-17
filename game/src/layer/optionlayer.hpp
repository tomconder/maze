#pragma once

#include "layer/scrollmenulayer.hpp"
#include "ui/checkbox.hpp"
#include "ui/confirmdialog.hpp"
#include "ui/selectlist.hpp"
#include "ui/slider.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

namespace game::layer {

// The Maximum Frame Rate slider runs over this range, in frames a second.
inline constexpr uint32_t minFrameRate     = 30;
inline constexpr uint32_t maxFrameRate     = 240;
inline constexpr uint32_t defaultFrameRate = 60;

enum class OptionMenuItem : uint8_t {
    GeneralHeading = 0,
    DisplayMode,
    AspectRatio,
    Resolution,
    VerticalSync,
    FrameRateCap,
    MaxFrameRate,
    AntiAliasing,
    QualityHeading,
    ShadowQuality,
    AdvancedHeading,
    AmbientOcclusion,
    Bloom,
    SubsurfaceScattering,
    Count,
};

class OptionLayer final : public ScrollMenuLayer {
public:
    OptionLayer();

    void onAttach() override;

private:
    // Every row applies as soon as it changes. A display mode change opens
    // the confirm layer and is undone unless it is accepted.
    bool pendingVsync                = false;
    bool pendingFrameRateCap         = false;
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
    std::unique_ptr<ui::Checkbox>   frameRateCapCheckbox;
    std::unique_ptr<ui::Slider>     frameRateSlider;
    std::unique_ptr<ui::Checkbox>   ambientOcclusionCheckbox;
    std::unique_ptr<ui::Checkbox>   bloomCheckbox;
    std::unique_ptr<ui::Checkbox>   subsurfaceScatteringCheckbox;

    // Widget backing a row, or nullptr when the row is not of that kind.
    ui::SelectList* listFor(OptionMenuItem item) const;
    ui::Checkbox*   checkboxFor(OptionMenuItem item) const;
    bool*           pendingFor(OptionMenuItem item);

    void cycleList(OptionMenuItem item, int delta);

    void togglePending(OptionMenuItem item);

    void filterResolutions();

    // The slider's frame rate. Applies it to the loop when the cap is on, and
    // saves it to Settings; save chooses whether the write reaches disk now
    // (a discrete step) or waits for the drag to end.
    uint32_t frameRate() const;
    void     applyFrameRate(bool save);

    // The mode asks to be kept; the resolution applies and saves at once.
    void applyDisplayMode();
    void applyResolution();

    // Keeps or undoes the mode change the dialog asked about. No-op while
    // Pending.
    void resolveDisplayChange(ui::ConfirmResult result);

    void syncPendingCheckboxState();

    void updateChangeStatus();

    void resetSelectionToCurrentState();

    void renderDots(size_t count, float valueCenterX, float rowY, float rowH,
                    size_t                       displayIdx,
                    const std::optional<size_t>& currentIdx) const;

    void renderRowLabel(size_t row, float y, std::string_view label,
                        const glm::vec3& color) const;

protected:
    bool takesInput() const override;

    bool isSelectable(size_t row) const override;

    void onOpened() override;

    void onClosed() override;

    void renderRow(size_t row, float x, float y, float w, float h) override;

    void onRowKeys(size_t                              row,
                   const sponge::input::InputSnapshot& input) override;

    bool onConfirm(size_t row) override;

    bool onRowPressed(size_t row, const glm::vec2& mouse) override;

    void onRowDrag(size_t row, const glm::vec2& mouse) override;

    void onRowReleased(size_t row) override;

    void onFontSizeChanged() override;

    void onResized(uint32_t width, uint32_t height) override;
};

}  // namespace game::layer
