#include "optionlayer.hpp"

#include "event/event.hpp"
#include "maze.hpp"
#include "resourcemanager.hpp"
#include "scene/orthocamera.hpp"
#include "sponge.hpp"
#include "ui/button.hpp"
#include "ui/checkbox.hpp"
#include "ui/selectlist.hpp"

#include <fmt/format.h>
#include <yoga/Yoga.h>

#include <array>
#include <cmath>
#include <memory>
#include <numeric>
#include <ranges>
#include <string>
#include <tuple>

namespace {
struct AspectRatioFilter {
    std::string_view label;
    uint32_t         numerator;
    uint32_t         denominator;
    bool             approximate = false;
};

constexpr auto aspectRatioFilters = std::to_array<AspectRatioFilter>({
    { .label = "4:3", .numerator = 4, .denominator = 3 },
    { .label = "5:3", .numerator = 5, .denominator = 3 },
    { .label = "5:4", .numerator = 5, .denominator = 4 },
    { .label = "16:9", .numerator = 16, .denominator = 9 },
    { .label       = "~16:9",
      .numerator   = 16,
      .denominator = 9,
      .approximate = true },
    { .label = "16:10", .numerator = 16, .denominator = 10 },
    { .label = "25:16", .numerator = 25, .denominator = 16 },
});

constexpr std::string_view returnMessage = "Return";
constexpr std::string_view applyMessage  = "Apply";

constexpr std::string_view cameraName = "intro";
constexpr std::string_view fontName   = "league-gothic";
constexpr std::string_view fontPath   = "/fonts/league-gothic.fnt";

constexpr glm::vec4 backgroundColor    = { 0.F, 0.F, 0.F, 1.F };
constexpr glm::vec4 buttonColor        = { 0.F, 0.F, 0.F, 0.F };
constexpr glm::vec4 hoverColor         = { 0.84F, 0.84F, 0.84F, 0.14F };
constexpr glm::vec3 textColor          = { 1.F, 1.F, 1.F };
constexpr glm::vec3 arrowDisabledColor = { 0.4F, 0.4F, 0.4F };
constexpr glm::vec4 textHoverColor     = { 0.84F, 0.04F, 0.04F, 0.14F };

constexpr uint32_t fontSize            = 48;
constexpr float    textMarginLeft      = 26.F;
constexpr float    cornerRadius        = 12.F;
constexpr float    selectedBorderWidth = 3.F;

std::tuple<float, float, float, float> getNodeLayout(const YGNodeRef node,
                                                     const float     offsetX,
                                                     const float     offsetY) {
    return { offsetX + YGNodeLayoutGetLeft(node),
             offsetY + YGNodeLayoutGetTop(node), YGNodeLayoutGetWidth(node),
             YGNodeLayoutGetHeight(node) };
}

inline std::string quadShaderName;
inline std::string fontShaderName;

std::unique_ptr<game::ui::Button> returnButton;

YGNodeRef aspectRatioNode    = nullptr;
YGNodeRef menuBackgroundNode = nullptr;
YGNodeRef menuNode           = nullptr;
YGNodeRef resolutionNode     = nullptr;
YGNodeRef verticalSyncNode   = nullptr;
YGNodeRef fullScreenNode     = nullptr;
YGNodeRef returnNode         = nullptr;
YGNodeRef rootNode           = nullptr;
YGNodeRef titleNode          = nullptr;

std::unique_ptr<sponge::platform::opengl::scene::Quad> quad;

std::shared_ptr<game::scene::OrthoCamera> orthoCamera;
}  // namespace

namespace game::layer {
using sponge::event::Event;
using sponge::event::EventDispatcher;
using sponge::event::KeyPressedEvent;
using sponge::event::MouseButtonPressedEvent;
using sponge::event::MouseMovedEvent;
using sponge::event::WindowResizeEvent;
using sponge::input::KeyCode;
using sponge::platform::glfw::core::Input;
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::FontCreateInfo;
using sponge::platform::opengl::scene::MSDFFont;
using sponge::platform::opengl::scene::Quad;

OptionLayer::OptionLayer() : Layer("options") {
    fontShaderName = MSDFFont::getShaderName();
    quadShaderName = Quad::getShaderName();
}

void OptionLayer::onAttach() {
    const auto fontNameStr = std::string(fontName);

    const auto fontCreateInfo =
        FontCreateInfo{ .name = fontNameStr, .path = std::string(fontPath) };
    AssetManager::createFont(fontCreateInfo);

    const auto orthoCameraCreateInfo =
        scene::OrthoCameraCreateInfo{ .name = std::string(cameraName) };
    orthoCamera = ResourceManager::createOrthoCamera(orthoCameraCreateInfo);

    quad = std::make_unique<Quad>();

    returnButton = std::make_unique<ui::Button>(
        ui::ButtonCreateInfo{ .topLeft      = glm::vec2{ 0.F },
                              .bottomRight  = glm::vec2{ 0.F },
                              .message      = std::string(returnMessage),
                              .fontSize     = 48,
                              .fontName     = fontNameStr,
                              .buttonColor  = buttonColor,
                              .textColor    = textColor,
                              .marginLeft   = 26,
                              .cornerRadius = 12.F,
                              .alignType = ui::ButtonAlignType::LeftAligned });

    for (const auto& shaderName : { fontShaderName, quadShaderName }) {
        const auto shader = AssetManager::getShader(shaderName);
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    rootNode = YGNodeNew();

    titleNode = YGNodeNew();
    YGNodeStyleSetFlexGrow(titleNode, 0.9F);
    YGNodeInsertChild(rootNode, titleNode, 0);

    menuNode = YGNodeNew();
    YGNodeStyleSetFlex(menuNode, 1.F);
    YGNodeStyleSetFlexDirection(menuNode, YGFlexDirectionRow);
    YGNodeInsertChild(rootNode, menuNode, 1);

    menuBackgroundNode = YGNodeNew();
    YGNodeStyleSetMargin(menuBackgroundNode, YGEdgeAll, 5.F);
    YGNodeStyleSetWidthPercent(menuBackgroundNode, 35.F);
    YGNodeInsertChild(menuNode, menuBackgroundNode, 0);

    auto makeMenuNode = [](const YGNodeRef parent, const int index) {
        auto* const child = YGNodeNew();
        YGNodeStyleSetFlex(child, 1.F);
        YGNodeStyleSetMargin(child, YGEdgeBottom, 30.F);
        YGNodeStyleSetMaxHeight(child, 110);
        YGNodeInsertChild(parent, child, index);
        return child;
    };

    aspectRatioNode  = makeMenuNode(menuBackgroundNode, 0);
    resolutionNode   = makeMenuNode(menuBackgroundNode, 1);
    fullScreenNode   = makeMenuNode(menuBackgroundNode, 2);
    verticalSyncNode = makeMenuNode(menuBackgroundNode, 3);
    returnNode       = makeMenuNode(menuBackgroundNode, 4);

    availableResolutions = Maze::get().getAvailableResolutions();

    // compute max display width across all aspect ratios and resolutions
    float maxCycleValueWidth;
    {
        const auto font    = AssetManager::getFont(fontName);
        maxCycleValueWidth = std::accumulate(
            aspectRatioFilters.begin(), aspectRatioFilters.end(), 0.F,
            [&](const float acc, const auto& f) {
                return std::max(acc, static_cast<float>(
                                         font->getLength(f.label, fontSize)));
            });
        maxCycleValueWidth = std::accumulate(
            availableResolutions.begin(), availableResolutions.end(),
            maxCycleValueWidth, [&](const float acc, const auto& res) {
                return std::max(
                    acc, static_cast<float>(font->getLength(
                             fmt::format("{} x {}", res.width, res.height),
                             fontSize)));
            });
    }

    const ui::SelectListCreateInfo selectCreateInfo{
        .fontName           = fontNameStr,
        .fontSize           = fontSize,
        .textColor          = textColor,
        .arrowDisabledColor = arrowDisabledColor,
        .textMarginLeft     = textMarginLeft,
        .maxValueWidth      = maxCycleValueWidth,
    };
    aspectRatioList = std::make_unique<ui::SelectList>(selectCreateInfo);
    resolutionList  = std::make_unique<ui::SelectList>(selectCreateInfo);

    constexpr ui::CheckboxCreateInfo checkboxCreateInfo{ .margin =
                                                             textMarginLeft };
    fullScreenCheckbox   = std::make_unique<ui::Checkbox>(checkboxCreateInfo);
    verticalSyncCheckbox = std::make_unique<ui::Checkbox>(checkboxCreateInfo);

    std::vector<std::string> arItems;
    arItems.reserve(aspectRatioFilters.size());
    for (const auto& f : aspectRatioFilters) {
        arItems.emplace_back(f.label);
    }
    aspectRatioList->setItems(std::move(arItems));

    const auto window        = Maze::get().getWindow();
    const auto currentWidth  = window->getWidth();
    const auto currentHeight = window->getHeight();

    const auto g  = std::gcd(currentWidth, currentHeight);
    const auto rw = currentWidth / g;
    const auto rh = currentHeight / g;

    const auto arIt =
        std::find_if(aspectRatioFilters.begin(), aspectRatioFilters.end(),
                     [&](const auto& f) {
                         const auto fg = std::gcd(f.numerator, f.denominator);
                         return !f.approximate && rw == f.numerator / fg &&
                                rh == f.denominator / fg;
                     });
    aspectRatioList->setSelectedIndex(
        arIt != aspectRatioFilters.end() ?
            static_cast<size_t>(
                std::distance(aspectRatioFilters.begin(), arIt)) :
            0);

    filterResolutions();

    const auto width  = static_cast<float>(orthoCamera->getWidth());
    const auto height = static_cast<float>(orthoCamera->getHeight());
    recalculateLayout(width, height);
}

void OptionLayer::onDetach() {
    YGNodeFreeRecursive(rootNode);
}

void OptionLayer::onEvent(Event& event) {
    EventDispatcher dispatcher(event);

    dispatcher.dispatch<KeyPressedEvent>(
        [this](const KeyPressedEvent& keyEvent) {
            return isActive() ? onKeyPressed(keyEvent) : false;
        });
    dispatcher.dispatch<MouseButtonPressedEvent>(
        [this](const MouseButtonPressedEvent& mouseEvent) {
            return isActive() ? onMouseButtonPressed(mouseEvent) : false;
        });
    dispatcher.dispatch<MouseMovedEvent>(
        [this](const MouseMovedEvent& mouseMovedEvent) {
            return isActive() ? onMouseMoved(mouseMovedEvent) : false;
        });
    dispatcher.dispatch<WindowResizeEvent>(
        [this](const WindowResizeEvent& windowResizeEvent) {
            return onWindowResize(windowResizeEvent);
        });
}

bool OptionLayer::onUpdate(const double elapsedTime) {
    UNUSED(elapsedTime);

    const auto width  = static_cast<float>(orthoCamera->getWidth());
    const auto height = static_cast<float>(orthoCamera->getHeight());
    quad->render({ 0.F, 0.F }, { width, height }, backgroundColor);

    auto [rootNodeX, rootNodeY, rootNodeW, rootNodeH] =
        getNodeLayout(rootNode, 0.F, 0.F);
    auto [menuNodeX, menuNodeY, menuNodeW, menuNodeH] =
        getNodeLayout(menuNode, rootNodeX, rootNodeY);
    auto [menuBgX, menuBgY, menuBgW, menuBgH] =
        getNodeLayout(menuBackgroundNode, menuNodeX, menuNodeY);

    const auto [arX, arY, arW, arH] =
        getNodeLayout(aspectRatioNode, menuBgX, menuBgY);
    const auto [resX, resY, resW, resH] =
        getNodeLayout(resolutionNode, menuBgX, menuBgY);
    const auto [fsX, fsY, fsW, fsH] =
        getNodeLayout(fullScreenNode, menuBgX, menuBgY);
    const auto [vsX, vsY, vsW, vsH] =
        getNodeLayout(verticalSyncNode, menuBgX, menuBgY);
    const auto [retX, retY, retW, retH] =
        getNodeLayout(returnNode, menuBgX, menuBgY);

    renderRowBackground(arX, arY, arW, arH, OptionMenuItem::AspectRatio);
    aspectRatioList->onUpdate(arX, arY, arW, arH, "Aspect Ratio");

    renderRowBackground(resX, resY, resW, resH, OptionMenuItem::Resolution);
    resolutionList->onUpdate(resX, resY, resW, resH, "Resolution");

    const auto rowFont        = AssetManager::getFont(fontName);
    auto       renderRowLabel = [&](const float x, const float y, const float h,
                              const std::string_view label) {
        const float textY = std::floor(
            y + (h - static_cast<float>(rowFont->getHeight(fontSize))) / 2.F);
        rowFont->render(label, { x + textMarginLeft, textY }, fontSize,
                              textColor);
    };

    renderRowBackground(fsX, fsY, fsW, fsH, OptionMenuItem::FullScreen);
    renderRowLabel(fsX, fsY, fsH, "Full Screen");
    fullScreenCheckbox->onUpdate(fsX, fsY, fsW, fsH,
                                 Maze::get().isFullscreen());

    renderRowBackground(vsX, vsY, vsW, vsH, OptionMenuItem::VerticalSync);
    renderRowLabel(vsX, vsY, vsH, "Vertical Sync");
    verticalSyncCheckbox->onUpdate(vsX, vsY, vsW, vsH,
                                   Maze::get().hasVerticalSync());

    returnButton->setPosition({ retX, retY }, { retX + retW, retY + retH });
    if (selectedItem == OptionMenuItem::Return) {
        returnButton->setBorderWidth(selectedBorderWidth);
        returnButton->setBorderColor(glm::vec4{ 1.F });
        returnButton->setButtonColor(textHoverColor);
    } else if (!returnButton->hasHover()) {
        returnButton->setBorderWidth(0.F);
        returnButton->setButtonColor(glm::vec4{ 0.F });
    } else {
        returnButton->setBorderWidth(0.F);
        returnButton->setButtonColor(hoverColor);
    }

    UNUSED(returnButton->onUpdate(elapsedTime));

    return true;
}

void OptionLayer::renderRowBackground(float x, float y, const float w,
                                      const float          h,
                                      const OptionMenuItem item) const {
    const bool isSelected = selectedItem == item;
    const bool isHovered  = hoveredItem == item;
    quad->render({ x, y }, { x + w, y + h },
                 isSelected ? textHoverColor :
                 isHovered  ? hoverColor :
                              buttonColor,
                 cornerRadius, isSelected ? selectedBorderWidth : 0.F,
                 glm::vec4{ 1.F });
}

void OptionLayer::renderRowText(const float x, float y, const float w,
                                const std::string_view label,
                                const std::string_view value) {
    const auto font     = AssetManager::getFont(fontName);
    const auto valueLen = static_cast<float>(font->getLength(value, fontSize));
    font->render(label, { x + textMarginLeft, y }, fontSize, textColor);
    font->render(value, { x + w - textMarginLeft - valueLen, y }, fontSize,
                 textColor);
}

void OptionLayer::recalculateLayout(const float width,
                                    const float height) const {
    YGNodeStyleSetWidth(rootNode, width);
    YGNodeStyleSetHeight(rootNode, height);
    YGNodeCalculateLayout(rootNode, width, height, YGDirectionLTR);
}

bool OptionLayer::onKeyPressed(const KeyPressedEvent& event) {
    const auto     keyCode   = event.getKeyCode();
    constexpr auto itemCount = +OptionMenuItem::Count;

    if (keyCode == KeyCode::SpongeKey_Escape) {
        clearHoveredItems();
        resetSelectionToCurrentState();
        setActive(false);
        return true;
    }

    if (keyCode == KeyCode::SpongeKey_Enter ||
        keyCode == KeyCode::SpongeKey_KPEnter ||
        keyCode == KeyCode::SpongeKey_Space) {
        if (selectedItem == OptionMenuItem::Return) {
            if (hasUnappliedChanges && !filteredResolutions.empty()) {
                const auto& res =
                    filteredResolutions[resolutionList->getSelectedIndex()];
                Maze::get().setResolution(res.width, res.height);
                hasUnappliedChanges = false;
                returnButton->setMessage(returnMessage);
            } else {
                clearHoveredItems();
                resetSelectionToCurrentState();
                setActive(false);
            }
        } else if (selectedItem == OptionMenuItem::FullScreen) {
            Maze::get().toggleFullscreen();
        } else if (selectedItem == OptionMenuItem::VerticalSync) {
            Maze::get().setVerticalSync(!Maze::get().hasVerticalSync());
        }
        return true;
    }

    if (keyCode == KeyCode::SpongeKey_Down ||
        keyCode == KeyCode::SpongeKey_KP2) {
        selectedItem =
            static_cast<OptionMenuItem>((+selectedItem + 1) % itemCount);
    }

    if (keyCode == KeyCode::SpongeKey_Up || keyCode == KeyCode::SpongeKey_KP8) {
        selectedItem = static_cast<OptionMenuItem>(
            (+selectedItem - 1 + itemCount) % itemCount);
    }

    if (selectedItem == OptionMenuItem::AspectRatio) {
        if (keyCode == KeyCode::SpongeKey_Left ||
            keyCode == KeyCode::SpongeKey_KP4) {
            aspectRatioList->selectPrev();
            filterResolutions();
        }

        if (keyCode == KeyCode::SpongeKey_Right ||
            keyCode == KeyCode::SpongeKey_KP6) {
            aspectRatioList->selectNext();
            filterResolutions();
        }
    }

    if (selectedItem == OptionMenuItem::Resolution &&
        !filteredResolutions.empty()) {
        if (keyCode == KeyCode::SpongeKey_Left ||
            keyCode == KeyCode::SpongeKey_KP4) {
            resolutionList->selectPrev();
            updateChangeStatus();
        }

        if (keyCode == KeyCode::SpongeKey_Right ||
            keyCode == KeyCode::SpongeKey_KP6) {
            resolutionList->selectNext();
            updateChangeStatus();
        }
    }

    return true;
}

bool OptionLayer::onMouseButtonPressed(const MouseButtonPressedEvent& event) {
    UNUSED(event);

    auto [mouseX, mouseY] = Input::getMousePosition();

    if (returnButton->isInside({ mouseX, mouseY })) {
        if (hasUnappliedChanges && !filteredResolutions.empty()) {
            const auto& res =
                filteredResolutions[resolutionList->getSelectedIndex()];
            Maze::get().setResolution(res.width, res.height);
            hasUnappliedChanges = false;
            returnButton->setMessage(returnMessage);
        } else {
            clearHoveredItems();
            resetSelectionToCurrentState();
            setActive(false);
        }
        return true;
    }

    auto [rootNodeX, rootNodeY, rootNodeW, rootNodeH] =
        getNodeLayout(rootNode, 0.F, 0.F);
    auto [menuNodeX, menuNodeY, menuNodeW, menuNodeH] =
        getNodeLayout(menuNode, rootNodeX, rootNodeY);
    auto [menuBackgroundNodeX, menuBackgroundNodeY, menuBackgroundNodeW,
          menuBackgroundNodeH] =
        getNodeLayout(menuBackgroundNode, menuNodeX, menuNodeY);

    const auto [arX, arY, arW, arH] = getNodeLayout(
        aspectRatioNode, menuBackgroundNodeX, menuBackgroundNodeY);

    if (mouseX >= arX && mouseX <= arX + arW && mouseY >= arY &&
        mouseY <= arY + arH) {
        selectedItem = OptionMenuItem::AspectRatio;

        if (aspectRatioList->isInsideLeft(mouseX, arX, arW)) {
            aspectRatioList->selectPrev();
            filterResolutions();
        } else if (aspectRatioList->isInsideRight(mouseX, arX, arW)) {
            aspectRatioList->selectNext();
            filterResolutions();
        }

        return true;
    }

    const auto [resX, resY, resW, resH] =
        getNodeLayout(resolutionNode, menuBackgroundNodeX, menuBackgroundNodeY);

    if (mouseX >= resX && mouseX <= resX + resW && mouseY >= resY &&
        mouseY <= resY + resH) {
        selectedItem = OptionMenuItem::Resolution;

        if (!filteredResolutions.empty()) {
            if (resolutionList->isInsideLeft(mouseX, resX, resW)) {
                resolutionList->selectPrev();
                updateChangeStatus();
            } else if (resolutionList->isInsideRight(mouseX, resX, resW)) {
                resolutionList->selectNext();
                updateChangeStatus();
            }
        }

        return true;
    }

    const auto [fullX, fullY, fullW, fullH] =
        getNodeLayout(fullScreenNode, menuBackgroundNodeX, menuBackgroundNodeY);

    if (mouseX >= fullX && mouseX <= fullX + fullW && mouseY >= fullY &&
        mouseY <= fullY + fullH) {
        selectedItem = OptionMenuItem::FullScreen;
        if (fullScreenCheckbox->isInside(mouseX, mouseY, fullX, fullY, fullW,
                                         fullH)) {
            Maze::get().toggleFullscreen();
        }
        return true;
    }

    const auto [vsyncX, vsyncY, vsyncW, vsyncH] = getNodeLayout(
        verticalSyncNode, menuBackgroundNodeX, menuBackgroundNodeY);

    if (mouseX >= vsyncX && mouseX <= vsyncX + vsyncW && mouseY >= vsyncY &&
        mouseY <= vsyncY + vsyncH) {
        selectedItem = OptionMenuItem::VerticalSync;
        if (verticalSyncCheckbox->isInside(mouseX, mouseY, vsyncX, vsyncY,
                                           vsyncW, vsyncH)) {
            Maze::get().setVerticalSync(!Maze::get().hasVerticalSync());
        }
        return true;
    }

    return true;
}

bool OptionLayer::onMouseMoved(const MouseMovedEvent& event) {
    const auto pos = glm::vec2{ event.getX(), event.getY() };

    auto updateHover = [&pos](ui::Button* button) {
        if (!button->hasHover() && button->isInside(pos)) {
            button->setHover(true);
        } else if (button->hasHover() && !button->isInside(pos)) {
            button->setHover(false);
        }
    };

    updateHover(returnButton.get());

    const auto rootNodeLayout = getNodeLayout(rootNode, 0.F, 0.F);
    const auto menuNodeLayout = getNodeLayout(
        menuNode, std::get<0>(rootNodeLayout), std::get<1>(rootNodeLayout));
    const auto menuBackgroundNodeLayout =
        getNodeLayout(menuBackgroundNode, std::get<0>(menuNodeLayout),
                      std::get<1>(menuNodeLayout));

    const float menuBackgroundNodeX = std::get<0>(menuBackgroundNodeLayout);
    const float menuBackgroundNodeY = std::get<1>(menuBackgroundNodeLayout);

    auto isOver = [&](auto* node) {
        const auto nodeLayout =
            getNodeLayout(node, menuBackgroundNodeX, menuBackgroundNodeY);
        const float x = std::get<0>(nodeLayout);
        const float y = std::get<1>(nodeLayout);
        const float w = std::get<2>(nodeLayout);
        const float h = std::get<3>(nodeLayout);
        return pos.x >= x && pos.x <= x + w && pos.y >= y && pos.y <= y + h;
    };

    if (isOver(aspectRatioNode)) {
        hoveredItem = OptionMenuItem::AspectRatio;
    } else if (isOver(resolutionNode)) {
        hoveredItem = OptionMenuItem::Resolution;
    } else if (isOver(fullScreenNode)) {
        hoveredItem = OptionMenuItem::FullScreen;
    } else if (isOver(verticalSyncNode)) {
        hoveredItem = OptionMenuItem::VerticalSync;
    } else {
        hoveredItem = std::nullopt;
    }

    return true;
}

bool OptionLayer::onWindowResize(const WindowResizeEvent& event) {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    for (const auto& shaderName : { fontShaderName, quadShaderName }) {
        const auto shader = AssetManager::getShader(shaderName);
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    const auto width  = static_cast<float>(event.getWidth());
    const auto height = static_cast<float>(event.getHeight());
    recalculateLayout(width, height);

    if (isActive()) {
        updateChangeStatus();
    }

    return false;
}

void OptionLayer::filterResolutions() {
    const auto& filter =
        aspectRatioFilters[aspectRatioList->getSelectedIndex()];
    auto isExactMatch = [&](const auto& res) {
        const auto g  = std::gcd(res.width, res.height);
        const auto fg = std::gcd(filter.numerator, filter.denominator);
        return res.width / g == filter.numerator / fg &&
               res.height / g == filter.denominator / fg;
    };

    auto matchesFilter = [&](const auto& res) {
        if (filter.approximate) {
            const float ratio =
                static_cast<float>(res.width) / static_cast<float>(res.height);
            const float target = static_cast<float>(filter.numerator) /
                                 static_cast<float>(filter.denominator);
            return !isExactMatch(res) &&
                   std::abs(ratio - target) / target <= 0.01F;
        }
        return isExactMatch(res);
    };

    auto filtered = availableResolutions | std::views::filter(matchesFilter);
    filteredResolutions =
        std::vector<sponge::core::Resolution>(filtered.begin(), filtered.end());

    const auto window        = Maze::get().getWindow();
    const auto currentWidth  = window->getWidth();
    const auto currentHeight = window->getHeight();

    if (filteredResolutions.empty()) {
        resolutionList->setItems(
            { fmt::format("{} x {}", currentWidth, currentHeight) });
        resolutionList->setSelectedIndex(0);
    } else {
        std::vector<std::string> resItems;
        resItems.reserve(filteredResolutions.size());
        for (const auto& res : filteredResolutions) {
            resItems.emplace_back(
                fmt::format("{} x {}", res.width, res.height));
        }
        resolutionList->setItems(std::move(resItems));

        const auto it = std::find_if(
            filteredResolutions.begin(), filteredResolutions.end(),
            [&](const auto& r) {
                return r.width == currentWidth && r.height == currentHeight;
            });
        resolutionList->setSelectedIndex(
            it != filteredResolutions.end() ?
                static_cast<size_t>(
                    std::distance(filteredResolutions.begin(), it)) :
                0);
    }

    updateChangeStatus();
}

void OptionLayer::updateChangeStatus() {
    if (filteredResolutions.empty()) {
        hasUnappliedChanges = false;
        returnButton->setMessage(returnMessage);
        return;
    }

    const auto  window = Maze::get().getWindow();
    const auto& res = filteredResolutions[resolutionList->getSelectedIndex()];
    hasUnappliedChanges =
        res.width != window->getWidth() || res.height != window->getHeight();

    returnButton->setMessage(hasUnappliedChanges ? applyMessage :
                                                   returnMessage);
}

void OptionLayer::clearHoveredItems() {
    returnButton->setHover(false);
    hoveredItem = std::nullopt;
}

void OptionLayer::resetSelectionToCurrentState() {
    const auto window        = Maze::get().getWindow();
    const auto currentWidth  = window->getWidth();
    const auto currentHeight = window->getHeight();

    const auto g  = std::gcd(currentWidth, currentHeight);
    const auto rw = currentWidth / g;
    const auto rh = currentHeight / g;

    const auto arIt =
        std::find_if(aspectRatioFilters.begin(), aspectRatioFilters.end(),
                     [&](const auto& f) {
                         const auto fg = std::gcd(f.numerator, f.denominator);
                         return !f.approximate && rw == f.numerator / fg &&
                                rh == f.denominator / fg;
                     });
    aspectRatioList->setSelectedIndex(
        arIt != aspectRatioFilters.end() ?
            static_cast<size_t>(
                std::distance(aspectRatioFilters.begin(), arIt)) :
            0);

    filterResolutions();
}
}  // namespace game::layer
