# Font Atlas Sizes & Responsive Menu Font Scaling

**Date:** 2026-06-10
**Status:** Approved

## Summary

Add 32px and 40px to the font atlas (currently only 16px and 48px), and make menu button font sizes responsive to window width using a shared helper function and a new `Button::setFontSize` API.

***

## Section 1: Font Atlas

**File:** `sponge/src/platform/opengl/scene/bitmapfont.cpp`

* Change `atlas.build({ { ttfPath } }, { 16, 48 })` → `atlas.build({ { ttfPath } }, { 16, 32, 40, 48 })`

**File:** `sponge/src/scene/fontatlas.cpp`

* Change `constexpr uint32_t kAtlasSize = 512` → `kAtlasSize = 1024`

**Rationale:** At 48px with ~96 glyphs, current atlas usage is ~200k pixels of a 262k-pixel (512×512) budget. Adding 32px (~68k) and 40px (~107k) pushes estimated total to ~375k, which exceeds 512×512. A 1024×1024 atlas provides ~1M pixels — ample headroom for all four sizes.

***

## Section 2: Shared Helper + Button API

**New file:** `game/src/ui/menufontsize.hpp`

A header-only free function used by all three layers:

```cpp
namespace game::ui {

inline uint32_t menuFontSizeForWidth(const uint32_t windowWidth) {
    if (windowWidth < 1024) return 32;
    if (windowWidth < 1440) return 40;
    return 48;
}

}  // namespace game::ui
```

**File:** `game/src/ui/button.hpp` / `button.cpp`

Add `void setFontSize(uint32_t size)` to `Button`. Implementation sets `textSize = size`.

***

## Section 3: Layer Integration

### All three layers (introlayer, exitlayer, optionlayer)

**`onAttach`:**

* Replace hardcoded `fontSize = 48` in `ButtonCreateInfo` with `menuFontSizeForWidth(orthoCamera->getWidth())`.

**`onWindowResize`:**

* After `recalculateLayout(...)`, call `button->setFontSize(menuFontSizeForWidth(event.getWidth()))` for every menu button in the layer.

### `optionlayer` additional changes

`optionlayer` uses `fontSize` in three more places:

1. `getLength(f.label, fontSize)` — width pre-computation in `onAttach`
2. `selectCreateInfo.fontSize` — SelectList creation
3. `renderRowLabel` lambda in `onUpdate` — inline rendering

To support responsive sizing in `onUpdate`, `OptionLayer` gains a private member `uint32_t currentFontSize` initialized in `onAttach` and updated in `onWindowResize`. The `onUpdate` `renderRowLabel` and `SelectList` rendering use `currentFontSize` instead of the hardcoded constant.

The `maxCycleValueWidth` pre-computation in `onAttach` uses the initial font size. On resize, `SelectList` internal width is updated via the existing `setItems`/`filterResolutions` path which already re-queries font metrics.

***

## Files Changed

| File | Change |
|---|---|
| `sponge/src/scene/fontatlas.cpp` | `kAtlasSize` 512 → 1024 |
| `sponge/src/platform/opengl/scene/bitmapfont.cpp` | Add sizes 32, 40 to atlas build |
| `game/src/ui/menufontsize.hpp` | New — shared breakpoint helper |
| `game/src/ui/button.hpp` | Add `setFontSize` declaration |
| `game/src/ui/button.cpp` | Add `setFontSize` implementation |
| `game/src/layer/introlayer.cpp` | Use `menuFontSizeForWidth`, update on resize |
| `game/src/layer/exitlayer.cpp` | Use `menuFontSizeForWidth`, update on resize |
| `game/src/layer/optionlayer.cpp` | Use `menuFontSizeForWidth`, `currentFontSize` member, update on resize |
| `game/src/layer/optionlayer.hpp` | Add `currentFontSize` member |

***

## Out of Scope

* The `statusFontSize = 16` in `introlayer` — always small UI chrome, no change needed.
* `SelectList` internal arrow sizing — unchanged.
* Atlas format, pixel format, or shader changes — none required.
