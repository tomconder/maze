# Font Atlas Sizes & Responsive Menu Font Scaling Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add 32px and 40px font sizes to the atlas and make all layer menu buttons scale their font size responsively based on window width.

**Architecture:** A header-only breakpoint helper (`menuFontSizeForWidth`) centralises the threshold logic. `Button` and `SelectList` each gain a `setFontSize` method. Each layer calls the helper on attach and on every window resize event. `OptionLayer` carries a `currentFontSize` member and a private helper that recomputes SelectList `maxValueWidth` so text never overflows its slot.

**Tech Stack:** C++20, FreeType 2, HarfBuzz, stb\_rect\_pack, CMake, MSVC (Windows).

***

## File Map

| File | Action | Responsibility |
|---|---|---|
| `sponge/src/scene/fontatlas.cpp` | Modify | `kAtlasSize` 512 → 1024 |
| `sponge/src/platform/opengl/scene/bitmapfont.cpp` | Modify | Add sizes 32, 40 to atlas build |
| `game/src/ui/menufontsize.hpp` | Create | Shared breakpoint helper |
| `game/src/ui/button.hpp` | Modify | Declare `setFontSize` |
| `game/src/ui/button.cpp` | Modify | Implement `setFontSize` |
| `game/src/ui/selectlist.hpp` | Modify | Declare `setFontSize` |
| `game/src/ui/selectlist.cpp` | Modify | Implement `setFontSize` (updates `leftLen`, `rightLen`) |
| `game/src/layer/introlayer.cpp` | Modify | Use helper in `onAttach` + `onWindowResize` |
| `game/src/layer/exitlayer.cpp` | Modify | Use helper in `onAttach` + `onWindowResize` |
| `game/src/layer/optionlayer.hpp` | Modify | Add `currentFontSize` member + `computeMaxCycleValueWidth` |
| `game/src/layer/optionlayer.cpp` | Modify | Full responsive wiring |

***

## Task 1: Grow the Font Atlas

**Files:**

* Modify: `sponge/src/scene/fontatlas.cpp`

* Modify: `sponge/src/platform/opengl/scene/bitmapfont.cpp`

* \[ ] **Step 1: Change atlas size constant**

In `sponge/src/scene/fontatlas.cpp`, change line 16:

```cpp
// Before:
constexpr uint32_t kAtlasSize  = 512;

// After:
constexpr uint32_t kAtlasSize  = 1024;
```

* \[ ] **Step 2: Add sizes 32 and 40 to the atlas build call**

In `sponge/src/platform/opengl/scene/bitmapfont.cpp`, change line 59:

```cpp
// Before:
atlas.build({ { ttfPath } }, { 16, 48 });

// After:
atlas.build({ { ttfPath } }, { 16, 32, 40, 48 });
```

* \[ ] **Step 3: Build and verify**

```bash
cmake --build out/build/windows-msvc-debug --target maze
```

Expected: build succeeds with no errors. Run the game and confirm text still renders correctly at existing sizes (16px status text bottom-right, 48px menu buttons).

* \[ ] **Step 4: Commit**

```bash
git add sponge/src/scene/fontatlas.cpp sponge/src/platform/opengl/scene/bitmapfont.cpp
git commit -m "feat: expand font atlas to 1024x1024 and add 32px and 40px sizes"
```

***

## Task 2: Shared Breakpoint Helper

**Files:**

* Create: `game/src/ui/menufontsize.hpp`

* \[ ] **Step 1: Create the helper header**

Create `game/src/ui/menufontsize.hpp` with this exact content:

```cpp
#pragma once

#include <cstdint>

namespace game::ui {

inline uint32_t menuFontSizeForWidth(const uint32_t windowWidth) {
    if (windowWidth < 1024) {
        return 32;
    }
    if (windowWidth < 1440) {
        return 40;
    }
    return 48;
}

}  // namespace game::ui
```

* \[ ] **Step 2: Build and verify**

```bash
cmake --build out/build/windows-msvc-debug --target maze
```

Expected: build succeeds (header is not yet included anywhere, so no compilation units change yet).

* \[ ] **Step 3: Commit**

```bash
git add game/src/ui/menufontsize.hpp
git commit -m "feat: add menuFontSizeForWidth responsive breakpoint helper"
```

***

## Task 3: Button::setFontSize

**Files:**

* Modify: `game/src/ui/button.hpp`

* Modify: `game/src/ui/button.cpp`

* \[ ] **Step 1: Declare `setFontSize` in the header**

In `game/src/ui/button.hpp`, add the declaration after `setMessage`:

```cpp
    void setMessage(std::string_view message);

    void setFontSize(uint32_t size);    // <-- add this line

    void setHover(const bool value) {
```

* \[ ] **Step 2: Implement `setFontSize` in button.cpp**

In `game/src/ui/button.cpp`, add the implementation after `setMessage` (after line 74):

```cpp
void Button::setFontSize(const uint32_t size) {
    if (textSize == size) {
        return;
    }
    textSize = size;
    length   = font->getLength(text, textSize);
    setPosition(top, bottom);
}
```

`setPosition` recomputes `textPosition` using the updated `textSize`, so vertical centering stays correct after the size change.

* \[ ] **Step 3: Build and verify**

```bash
cmake --build out/build/windows-msvc-debug --target maze
```

Expected: build succeeds.

* \[ ] **Step 4: Commit**

```bash
git add game/src/ui/button.hpp game/src/ui/button.cpp
git commit -m "feat: add Button::setFontSize for responsive font scaling"
```

***

## Task 4: SelectList::setFontSize

**Files:**

* Modify: `game/src/ui/selectlist.hpp`
* Modify: `game/src/ui/selectlist.cpp`

Context: `SelectList` caches `leftLen` and `rightLen` (pixel widths of `"< "` and `" >"`) from the font at construction. These must be recomputed whenever `fontSize` changes, or arrow hit-testing and layout math will use stale values.

* \[ ] **Step 1: Declare `setFontSize` in selectlist.hpp**

In `game/src/ui/selectlist.hpp`, add the declaration after `setMaxValueWidth`:

```cpp
    void setItems(std::vector<std::string> items);
    void setMaxValueWidth(float width);
    void setFontSize(uint32_t size);    // <-- add this line
```

* \[ ] **Step 2: Implement `setFontSize` in selectlist.cpp**

In `game/src/ui/selectlist.cpp`, add the implementation after `setMaxValueWidth` (after line 34):

```cpp
void SelectList::setFontSize(const uint32_t size) {
    if (fontSize == size) {
        return;
    }
    fontSize = size;
    leftLen  = static_cast<float>(font->getLength(leftArrow, fontSize));
    rightLen = static_cast<float>(font->getLength(rightArrow, fontSize));
}
```

* \[ ] **Step 3: Build and verify**

```bash
cmake --build out/build/windows-msvc-debug --target maze
```

Expected: build succeeds.

* \[ ] **Step 4: Commit**

```bash
git add game/src/ui/selectlist.hpp game/src/ui/selectlist.cpp
git commit -m "feat: add SelectList::setFontSize, recomputes cached arrow lengths"
```

***

## Task 5: Update IntroLayer

**Files:**

* Modify: `game/src/layer/introlayer.cpp`

* \[ ] **Step 1: Add the include**

At the top of `game/src/layer/introlayer.cpp`, add the include after the existing `#include "ui/button.hpp"`:

```cpp
#include "ui/button.hpp"
#include "ui/menufontsize.hpp"
```

* \[ ] **Step 2: Replace hardcoded font size in `onAttach`**

In `onAttach`, the lambda `makeMenuButton` currently passes `fontSize = 48`. Change it to use the helper. The lambda is defined starting around line 89. Change the `ButtonCreateInfo` inside it:

```cpp
    auto makeMenuButton = [fontNameStr, this](std::string_view message) {
        return std::make_unique<ui::Button>(ui::ButtonCreateInfo{
            .topLeft      = glm::vec2{ 0.F },
            .bottomRight  = glm::vec2{ 0.F },
            .message      = std::string(message),
            .fontSize     = ui::menuFontSizeForWidth(
                                static_cast<uint32_t>(orthoCamera->getWidth())),
            .fontName     = fontNameStr,
            .buttonColor  = buttonColor,
            .textColor    = textColor,
            .marginLeft   = 26,
            .cornerRadius = 12.F,
            .alignType    = ui::ButtonAlignType::LeftAligned });
    };
```

Note: the capture list adds `this` so `orthoCamera` is accessible.

* \[ ] **Step 3: Update font size in `onWindowResize`**

In `IntroLayer::onWindowResize`, add size updates after `recalculateLayout`:

```cpp
bool IntroLayer::onWindowResize(const WindowResizeEvent& event) {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    const auto [width, height] =
        std::pair{ static_cast<float>(event.getWidth()),
                   static_cast<float>(event.getHeight()) };
    recalculateLayout(width, height);

    const auto newFontSize = ui::menuFontSizeForWidth(event.getWidth());
    newGameButton->setFontSize(newFontSize);
    optionsButton->setFontSize(newFontSize);
    quitButton->setFontSize(newFontSize);

    return false;
}
```

* \[ ] **Step 4: Build and verify**

```bash
cmake --build out/build/windows-msvc-debug --target maze
```

Expected: build succeeds. Run the game, open the intro menu. Resize the window across the 1024 and 1440 breakpoints and confirm the button text changes size.

* \[ ] **Step 5: Commit**

```bash
git add game/src/layer/introlayer.cpp
git commit -m "feat: scale IntroLayer menu font size with window width"
```

***

## Task 6: Update ExitLayer

**Files:**

* Modify: `game/src/layer/exitlayer.cpp`

* \[ ] **Step 1: Add the include**

In `game/src/layer/exitlayer.cpp`, add after `#include "ui/button.hpp"`:

```cpp
#include "ui/button.hpp"
#include "ui/menufontsize.hpp"
```

* \[ ] **Step 2: Replace hardcoded font size in `onAttach`**

The lambda `makeMenuButton` currently passes `fontSize = 48` (around line 81). Change it:

```cpp
    auto makeMenuButton = [fontNameStr, this](std::string_view message) {
        return std::make_unique<ui::Button>(ui::ButtonCreateInfo{
            .topLeft      = glm::vec2{ 0.F },
            .bottomRight  = glm::vec2{ 0.F },
            .message      = std::string(message),
            .fontSize     = ui::menuFontSizeForWidth(
                                static_cast<uint32_t>(orthoCamera->getWidth())),
            .fontName     = fontNameStr,
            .buttonColor  = buttonColor,
            .textColor    = textColor,
            .marginLeft   = 26,
            .cornerRadius = 12.F,
            .alignType    = ui::ButtonAlignType::LeftAligned });
    };
```

Note: add `this` to the capture list so `orthoCamera` is accessible.

* \[ ] **Step 3: Update font size in `onWindowResize`**

In `ExitLayer::onWindowResize`, add size updates after `recalculateLayout`:

```cpp
bool ExitLayer::onWindowResize(const WindowResizeEvent& event) {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    const auto [width, height] =
        std::pair{ static_cast<float>(event.getWidth()),
                   static_cast<float>(event.getHeight()) };
    recalculateLayout(width, height);

    const auto newFontSize = ui::menuFontSizeForWidth(event.getWidth());
    continueButton->setFontSize(newFontSize);
    optionsButton->setFontSize(newFontSize);
    returnToMenuButton->setFontSize(newFontSize);
    exitButton->setFontSize(newFontSize);

    return false;
}
```

* \[ ] **Step 4: Build and verify**

```bash
cmake --build out/build/windows-msvc-debug --target maze
```

Expected: build succeeds. Run the game, press Escape to open the exit menu, resize the window across breakpoints and verify font size changes.

* \[ ] **Step 5: Commit**

```bash
git add game/src/layer/exitlayer.cpp
git commit -m "feat: scale ExitLayer menu font size with window width"
```

***

## Task 7: Update OptionLayer

**Files:**

* Modify: `game/src/layer/optionlayer.hpp`
* Modify: `game/src/layer/optionlayer.cpp`

This is the most involved layer. `OptionLayer` uses `fontSize` in five places: the `returnButton` `ButtonCreateInfo`, the `selectCreateInfo`, the `maxCycleValueWidth` computation, and the `renderRowLabel` lambda in `onUpdate`. Additionally, `maxValueWidth` must be recomputed whenever the font size changes upward, or 48px text will overflow a slot sized for 32px.

* \[ ] **Step 1: Update optionlayer.hpp — add member and private helper**

In `game/src/layer/optionlayer.hpp`, add `currentFontSize` to the private section and declare `computeMaxCycleValueWidth`:

```cpp
private:
    OptionMenuItem                selectedItem = OptionMenuItem::AspectRatio;
    std::optional<OptionMenuItem> hoveredItem;

    uint32_t currentFontSize = 48;    // <-- add this line

    bool hasUnappliedChanges   = false;
    bool wasActiveLastFrame    = false;
    bool waitForConfirmRelease = false;

    bool pendingFullscreen = false;
    bool pendingVsync      = false;
    bool pendingFxaa       = false;

    std::unique_ptr<ui::SelectList> aspectRatioList;
    std::unique_ptr<ui::SelectList> resolutionList;
    std::unique_ptr<ui::Checkbox>   antiAliasingCheckbox;
    std::unique_ptr<ui::Checkbox>   fullScreenCheckbox;
    std::unique_ptr<ui::Checkbox>   verticalSyncCheckbox;

    void renderRowBackground(float x, float y, float w, float h,
                             OptionMenuItem item) const;

    float computeMaxCycleValueWidth(uint32_t size) const;    // <-- add this

    void filterResolutions();
    // ... rest unchanged
```

* \[ ] **Step 2: Add the includes in optionlayer.cpp**

In `game/src/layer/optionlayer.cpp`, add after `#include "ui/selectlist.hpp"`:

```cpp
#include "ui/selectlist.hpp"
#include "ui/menufontsize.hpp"
```

* \[ ] **Step 3: Remove the file-scope `fontSize` constant**

In `game/src/layer/optionlayer.cpp`, in the anonymous namespace (around line 60), **remove** this line:

```cpp
constexpr uint32_t fontSize            = 48;
```

It is replaced by the `currentFontSize` member.

* \[ ] **Step 4: Add `computeMaxCycleValueWidth` implementation**

Add this method to `game/src/layer/optionlayer.cpp` before `OptionLayer::onAttach`:

```cpp
float OptionLayer::computeMaxCycleValueWidth(const uint32_t size) const {
    const auto font = AssetManager::getFont(fontName);
    float maxWidth = std::accumulate(
        aspectRatioFilters.begin(), aspectRatioFilters.end(), 0.F,
        [&](const float acc, const auto& f) {
            return std::max(acc,
                            static_cast<float>(font->getLength(f.label, size)));
        });
    maxWidth = std::accumulate(
        availableResolutions.begin(), availableResolutions.end(), maxWidth,
        [&](const float acc, const auto& res) {
            return std::max(acc,
                            static_cast<float>(font->getLength(
                                fmt::format("{} \xc3\x97 {}", res.width,
                                            res.height),
                                size)));
        });
    return maxWidth;
}
```

Note: `\xc3\x97` is UTF-8 for `×` (the multiplication sign used in resolution strings).

* \[ ] **Step 5: Update `onAttach` to use `currentFontSize`**

In `OptionLayer::onAttach`, replace the block that computes `maxCycleValueWidth` inline, and replace all uses of the removed `fontSize` constant with `currentFontSize`.

Replace the `returnButton` creation (around line 138):

```cpp
    currentFontSize = ui::menuFontSizeForWidth(
        static_cast<uint32_t>(orthoCamera->getWidth()));

    returnButton = std::make_unique<ui::Button>(
        ui::ButtonCreateInfo{ .topLeft      = glm::vec2{ 0.F },
                              .bottomRight  = glm::vec2{ 0.F },
                              .message      = std::string(returnMessage),
                              .fontSize     = currentFontSize,
                              .fontName     = fontNameStr,
                              .buttonColor  = buttonColor,
                              .textColor    = textColor,
                              .marginLeft   = 26,
                              .cornerRadius = 12.F,
                              .alignType = ui::ButtonAlignType::LeftAligned });
```

Replace the `maxCycleValueWidth` inline block (the block with the two `std::accumulate` calls) with a single call:

```cpp
    availableResolutions = Maze::get().getAvailableResolutions();

    const float maxCycleValueWidth = computeMaxCycleValueWidth(currentFontSize);
```

Replace the `selectCreateInfo` struct (around line 210):

```cpp
    const ui::SelectListCreateInfo selectCreateInfo{
        .fontName           = fontNameStr,
        .fontSize           = currentFontSize,
        .textColor          = textColor,
        .arrowDisabledColor = arrowDisabledColor,
        .textMarginLeft     = textMarginLeft,
        .maxValueWidth      = maxCycleValueWidth,
    };
```

* \[ ] **Step 6: Update `onUpdate` to use `currentFontSize`**

In `OptionLayer::onUpdate`, the `renderRowLabel` lambda uses `fontSize` (which no longer exists). Change it to use `currentFontSize`:

```cpp
    auto renderRowLabel = [&](const float x, const float y, const float h,
                              const std::string_view label) {
        const float textY = std::floor(
            y + (h - static_cast<float>(rowFont->getHeight(currentFontSize))) /
                    2.F);
        rowFont->beginPass(currentFontSize);
        rowFont->render(label, { x + textMarginLeft, textY }, textColor);
        rowFont->endPass();
    };
```

* \[ ] **Step 7: Update `onWindowResize` to propagate the new font size**

Replace the full `OptionLayer::onWindowResize` body:

```cpp
bool OptionLayer::onWindowResize(const WindowResizeEvent& event) {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    const auto width  = static_cast<float>(event.getWidth());
    const auto height = static_cast<float>(event.getHeight());
    recalculateLayout(width, height);

    const auto newFontSize = ui::menuFontSizeForWidth(event.getWidth());
    if (newFontSize != currentFontSize) {
        currentFontSize = newFontSize;

        const float newMaxWidth = computeMaxCycleValueWidth(currentFontSize);
        aspectRatioList->setFontSize(currentFontSize);
        aspectRatioList->setMaxValueWidth(newMaxWidth);
        resolutionList->setFontSize(currentFontSize);
        resolutionList->setMaxValueWidth(newMaxWidth);

        returnButton->setFontSize(currentFontSize);
    }

    if (isActive()) {
        updateChangeStatus();
    }

    return false;
}
```

* \[ ] **Step 8: Build and verify**

```bash
cmake --build out/build/windows-msvc-debug --target maze
```

Expected: build succeeds with no errors or warnings about `fontSize` being undefined. Run the game, open Options, resize across breakpoints — all labels (row labels, SelectList label + arrows + values, Return button) should change size together.

* \[ ] **Step 9: Commit**

```bash
git add game/src/layer/optionlayer.hpp game/src/layer/optionlayer.cpp
git commit -m "feat: scale OptionLayer menu font size with window width"
```

***

## Self-Review Checklist

**Spec coverage:**

* \[x] Atlas kAtlasSize 512 → 1024 — Task 1
* \[x] Add 32px, 40px to build call — Task 1
* \[x] `menufontsize.hpp` helper — Task 2
* \[x] `Button::setFontSize` — Task 3
* \[x] `SelectList::setFontSize` + `leftLen`/`rightLen` recompute — Task 4
* \[x] IntroLayer `onAttach` + `onWindowResize` — Task 5
* \[x] ExitLayer `onAttach` + `onWindowResize` — Task 6
* \[x] OptionLayer `currentFontSize` member — Task 7 Step 1
* \[x] OptionLayer `computeMaxCycleValueWidth` — Task 7 Steps 4–5
* \[x] OptionLayer `onUpdate` `renderRowLabel` — Task 7 Step 6
* \[x] OptionLayer `onWindowResize` — Task 7 Step 7

**Type consistency:** `menuFontSizeForWidth` returns `uint32_t`; all `setFontSize` methods take `uint32_t`; `currentFontSize` is `uint32_t` — consistent throughout.

**No placeholders:** All steps include exact code. No TBDs.
