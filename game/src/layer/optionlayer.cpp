#include "optionlayer.hpp"

#include "event/event.hpp"
#include "maze.hpp"
#include "resourcemanager.hpp"
#include "scene/orthocamera.hpp"
#include "sponge.hpp"
#include "ui/button.hpp"

#include <fmt/format.h>
#include <yoga/Yoga.h>

#include <array>
#include <cassert>
#include <memory>
#include <numeric>
#include <ranges>
#include <string>

namespace {
struct AspectRatioFilter {
    std::string_view label;
    uint32_t         numerator;
    uint32_t         denominator;
    bool             approximate = false;
};

constexpr auto aspectRatioFilters = std::to_array<AspectRatioFilter>({
    { .label = "4:3", .numerator = 4, .denominator = 3, .approximate = false },
    { .label = "5:3", .numerator = 5, .denominator = 3, .approximate = false },
    { .label = "5:4", .numerator = 5, .denominator = 4, .approximate = false },
    { .label       = "16:9",
      .numerator   = 16,
      .denominator = 9,
      .approximate = false },
    { .label       = "~16:9",
      .numerator   = 16,
      .denominator = 9,
      .approximate = true },
    { .label       = "16:10",
      .numerator   = 16,
      .denominator = 10,
      .approximate = false },
    { .label       = "25:16",
      .numerator   = 25,
      .denominator = 16,
      .approximate = false },
});

constexpr std::string_view returnMessage = "Return";
constexpr std::string_view applyMessage  = "Apply";

constexpr std::string_view cameraName = "intro";
constexpr std::string_view fontName   = "league-gothic";
constexpr std::string_view fontPath   = "/fonts/league-gothic.fnt";

constexpr glm::vec4 backgroundColor = { 0.F, 0.F, 0.F, 1.F };
constexpr glm::vec4 buttonColor     = { 0.F, 0.F, 0.F, 0.F };
constexpr glm::vec4 hoverColor      = { 0.84F, 0.84F, 0.84F, 0.14F };
constexpr glm::vec3 textColor       = { 1.F, 1.F, 1.F };
constexpr glm::vec4 textHoverColor  = { 0.84F, 0.04F, 0.04F, 0.14F };

constexpr uint32_t fontSize            = 48;
constexpr float    textMarginLeft      = 56.F;
constexpr float    cornerRadius        = 12.F;
constexpr float    selectedBorderWidth = 3.F;

std::tuple<float, float, float, float>
    getNodeLayout(const YGNodeRef node, float offsetX, float offsetY) {
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
                              .marginLeft   = 56,
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

    auto makeMenuNode = [](YGNodeRef parent, int index) {
        auto* const node = YGNodeNew();
        YGNodeStyleSetFlex(node, 1.F);
        YGNodeStyleSetMargin(node, YGEdgeBottom, 30.F);
        YGNodeStyleSetMaxHeight(node, 110);
        YGNodeInsertChild(parent, node, index);
        return node;
    };

    aspectRatioNode  = makeMenuNode(menuBackgroundNode, 0);
    resolutionNode   = makeMenuNode(menuBackgroundNode, 1);
    fullScreenNode   = makeMenuNode(menuBackgroundNode, 2);
    verticalSyncNode = makeMenuNode(menuBackgroundNode, 3);
    returnNode       = makeMenuNode(menuBackgroundNode, 4);

    availableResolutions = Maze::get().getAvailableResolutions();

    const auto window        = Maze::get().getWindow();
    const auto currentWidth  = window->getWidth();
    const auto currentHeight = window->getHeight();

    // Auto-detect the initial aspect ratio from the current window size
    const auto g  = std::gcd(currentWidth, currentHeight);
    const auto rw = currentWidth / g;
    const auto rh = currentHeight / g;

    const auto arIt =
        std::ranges::find_if(aspectRatioFilters, [&](const auto& f) {
            const auto fg = std::gcd(f.numerator, f.denominator);
            return !f.approximate && rw == f.numerator / fg &&
                   rh == f.denominator / fg;
        });
    selectedAspectRatioIndex = arIt != aspectRatioFilters.end() ?
                                   static_cast<size_t>(std::ranges::distance(
                                       aspectRatioFilters.begin(), arIt)) :
                                   0;

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

    dispatcher.dispatch<KeyPressedEvent>([this](const KeyPressedEvent& event) {
        return isActive() ? onKeyPressed(event) : false;
    });
    dispatcher.dispatch<MouseButtonPressedEvent>(
        [this](const MouseButtonPressedEvent& event) {
            return isActive() ? onMouseButtonPressed(event) : false;
        });
    dispatcher.dispatch<MouseMovedEvent>([this](const MouseMovedEvent& event) {
        return isActive() ? onMouseMoved(event) : false;
    });
    dispatcher.dispatch<WindowResizeEvent>(
        [this](const WindowResizeEvent& event) {
            return onWindowResize(event);
        });
}

bool OptionLayer::onUpdate(double elapsedTime) {
    UNUSED(elapsedTime);

    const auto width  = static_cast<float>(orthoCamera->getWidth());
    const auto height = static_cast<float>(orthoCamera->getHeight());

    quad->render({ 0.F, 0.F }, { width, height }, backgroundColor);

    auto [rootNodeX, rootNodeY, rootNodeW, rootNodeH] =
        getNodeLayout(rootNode, 0.F, 0.F);
    auto [menuNodeX, menuNodeY, menuNodeW, menuNodeH] =
        getNodeLayout(menuNode, rootNodeX, rootNodeY);
    auto [menuBackgroundNodeX, menuBackgroundNodeY, menuBackgroundNodeW,
          menuBackgroundNodeH] =
        getNodeLayout(menuBackgroundNode, menuNodeX, menuNodeY);

    const auto [aspectRatioX, aspectRatioY, aspectRatioW, aspectRatioH] =
        getNodeLayout(aspectRatioNode, menuBackgroundNodeX,
                      menuBackgroundNodeY);
    const auto [resolutionX, resolutionY, resolutionW, resolutionH] =
        getNodeLayout(resolutionNode, menuBackgroundNodeX, menuBackgroundNodeY);
    const auto [fullScreenX, fullScreenY, fullScreenW, fullScreenH] =
        getNodeLayout(fullScreenNode, menuBackgroundNodeX, menuBackgroundNodeY);
    const auto [verticalSyncX, verticalSyncY, verticalSyncW, verticalSyncH] =
        getNodeLayout(verticalSyncNode, menuBackgroundNodeX,
                      menuBackgroundNodeY);
    const auto [returnX, returnY, returnW, returnH] =
        getNodeLayout(returnNode, menuBackgroundNodeX, menuBackgroundNodeY);

    const auto font = AssetManager::getFont(fontName);

    assert(selectedAspectRatioIndex < aspectRatioFilters.size());
    const auto& arFilter = aspectRatioFilters[selectedAspectRatioIndex];
    const char* arLeft   = selectedAspectRatioIndex > 0 ? "<" : " ";
    const char* arRight =
        selectedAspectRatioIndex + 1 < aspectRatioFilters.size() ? ">" : " ";
    const std::string aspectRatioStr =
        fmt::format("Aspect Ratio: {} {} {}", arLeft, arFilter.label, arRight);
    const bool isAspectRatioSelected =
        selectedItem == OptionMenuItem::AspectRatio;
    const bool isAspectRatioHovered =
        hoveredItem == OptionMenuItem::AspectRatio;
    quad->render(
        { aspectRatioX, aspectRatioY },
        { aspectRatioX + aspectRatioW, aspectRatioY + aspectRatioH },
        isAspectRatioSelected ? textHoverColor
        : isAspectRatioHovered ? hoverColor
                               : buttonColor,
        cornerRadius, isAspectRatioSelected ? selectedBorderWidth : 0.F,
        glm::vec4{ 1.F });
    font->render(aspectRatioStr,
                 { aspectRatioX + textMarginLeft, aspectRatioY }, fontSize,
                 textColor);

    std::string resolutionStr;
    if (!filteredResolutions.empty()) {
        assert(selectedResolutionIndex < filteredResolutions.size());
        const auto& res     = filteredResolutions[selectedResolutionIndex];
        const char* resLeft = selectedResolutionIndex > 0 ? "<" : " ";
        const char* resRight =
            selectedResolutionIndex + 1 < filteredResolutions.size() ? ">" :
                                                                       " ";
        resolutionStr = fmt::format("Resolution: {} {}x{} {}", resLeft,
                                    res.width, res.height, resRight);
    } else {
        const auto window = Maze::get().getWindow();
        resolutionStr     = fmt::format("Resolution: {}x{}", window->getWidth(),
                                        window->getHeight());
    }

    const bool isResolutionSelected =
        selectedItem == OptionMenuItem::Resolution;
    const bool isResolutionHovered = hoveredItem == OptionMenuItem::Resolution;
    quad->render({ resolutionX, resolutionY },
                 { resolutionX + resolutionW, resolutionY + resolutionH },
                 isResolutionSelected ? textHoverColor
                 : isResolutionHovered ? hoverColor
                                      : buttonColor,
                 cornerRadius, isResolutionSelected ? selectedBorderWidth : 0.F,
                 glm::vec4{ 1.F });
    font->render(resolutionStr, { resolutionX + textMarginLeft, resolutionY },
                 fontSize, textColor);

    const bool isFullScreenSelected =
        selectedItem == OptionMenuItem::FullScreen;
    const bool isFullScreenHovered = hoveredItem == OptionMenuItem::FullScreen;
    quad->render({ fullScreenX, fullScreenY },
                 { fullScreenX + fullScreenW, fullScreenY + fullScreenH },
                 isFullScreenSelected ? textHoverColor
                 : isFullScreenHovered ? hoverColor
                                      : buttonColor,
                 cornerRadius, isFullScreenSelected ? selectedBorderWidth : 0.F,
                 glm::vec4{ 1.F });
    const std::string fullScreenMessageStr = fmt::format(
        "Full Screen: {}", Maze::get().isFullscreen() ? "True" : "False");
    font->render(fullScreenMessageStr,
                 { fullScreenX + textMarginLeft, fullScreenY }, fontSize,
                 textColor);

    const bool isVerticalSyncSelected =
        selectedItem == OptionMenuItem::VerticalSync;
    const bool isVerticalSyncHovered =
        hoveredItem == OptionMenuItem::VerticalSync;
    quad->render(
        { verticalSyncX, verticalSyncY },
        { verticalSyncX + verticalSyncW, verticalSyncY + verticalSyncH },
        isVerticalSyncSelected ? textHoverColor
        : isVerticalSyncHovered ? hoverColor
                                : buttonColor,
        cornerRadius, isVerticalSyncSelected ? selectedBorderWidth : 0.F,
        glm::vec4{ 1.F });
    const std::string verticalSyncMessageStr = fmt::format(
        "Vertical Sync: {}", Maze::get().hasVerticalSync() ? "True" : "False");
    font->render(verticalSyncMessageStr,
                 { verticalSyncX + textMarginLeft, verticalSyncY }, fontSize,
                 textColor);

    returnButton->setPosition({ returnX, returnY },
                              { returnX + returnW, returnY + returnH });

    auto updateButtonVisuals = [this](ui::Button* button, OptionMenuItem item) {
        if (selectedItem == item) {
            button->setBorderWidth(selectedBorderWidth);
            button->setBorderColor(glm::vec4{ 1.F });
            button->setButtonColor(textHoverColor);
        } else if (!button->hasHover()) {
            button->setBorderWidth(0.F);
            button->setButtonColor(glm::vec4{ 0.F });
        } else {
            button->setBorderWidth(0.F);
            button->setButtonColor(hoverColor);
        }
    };

    updateButtonVisuals(returnButton.get(), OptionMenuItem::Return);

    UNUSED(returnButton->onUpdate(elapsedTime));

    return true;
}

void OptionLayer::recalculateLayout(float width, float height) const {
    YGNodeStyleSetWidth(rootNode, width);
    YGNodeStyleSetHeight(rootNode, height);
    YGNodeCalculateLayout(rootNode, width, height, YGDirectionLTR);
}

bool OptionLayer::onKeyPressed(const KeyPressedEvent& event) {
    const auto     keyCode   = event.getKeyCode();
    constexpr auto itemCount = +OptionMenuItem::Count;

    if (keyCode == KeyCode::SpongeKey_Escape) {
        clearHoveredItems();
        hasUnappliedChanges = false;
        setActive(false);
        return true;
    }

    if (keyCode == KeyCode::SpongeKey_Enter ||
        keyCode == KeyCode::SpongeKey_KPEnter) {
        if (selectedItem == OptionMenuItem::Return) {
            if (hasUnappliedChanges && !filteredResolutions.empty()) {
                assert(selectedResolutionIndex < filteredResolutions.size());
                const auto& res = filteredResolutions[selectedResolutionIndex];
                Maze::get().setResolution(res.width, res.height);
                hasUnappliedChanges = false;
            } else {
                clearHoveredItems();
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
        constexpr auto aspectRatioCount = aspectRatioFilters.size();

        if (keyCode == KeyCode::SpongeKey_Left ||
            keyCode == KeyCode::SpongeKey_KP4) {
            if (selectedAspectRatioIndex > 0) {
                selectedAspectRatioIndex--;
            }
            filterResolutions();
        }

        if (keyCode == KeyCode::SpongeKey_Right ||
            keyCode == KeyCode::SpongeKey_KP6) {
            if (selectedAspectRatioIndex + 1 < aspectRatioCount) {
                selectedAspectRatioIndex++;
            }
            filterResolutions();
        }
    }

    if (selectedItem == OptionMenuItem::Resolution &&
        !filteredResolutions.empty()) {
        const auto resolutionCount = filteredResolutions.size();

        if (keyCode == KeyCode::SpongeKey_Left ||
            keyCode == KeyCode::SpongeKey_KP4) {
            if (selectedResolutionIndex > 0) {
                selectedResolutionIndex--;
            }
            updateChangeStatus();
        }

        if (keyCode == KeyCode::SpongeKey_Right ||
            keyCode == KeyCode::SpongeKey_KP6) {
            if (selectedResolutionIndex + 1 < resolutionCount) {
                selectedResolutionIndex++;
            }
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
            assert(selectedResolutionIndex < filteredResolutions.size());
            const auto& res = filteredResolutions[selectedResolutionIndex];
            Maze::get().setResolution(res.width, res.height);
            hasUnappliedChanges = false;
        } else {
            clearHoveredItems();
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
        constexpr auto aspectRatioCount = aspectRatioFilters.size();
        const auto     midX             = arX + arW / 2.F;

        if (mouseX < midX) {
            if (selectedAspectRatioIndex > 0) {
                selectedAspectRatioIndex--;
            }
        } else {
            if (selectedAspectRatioIndex + 1 < aspectRatioCount) {
                selectedAspectRatioIndex++;
            }
        }

        selectedItem = OptionMenuItem::AspectRatio;
        filterResolutions();

        return true;
    }

    const auto [resX, resY, resW, resH] =
        getNodeLayout(resolutionNode, menuBackgroundNodeX, menuBackgroundNodeY);

    if (mouseX >= resX && mouseX <= resX + resW && mouseY >= resY &&
        mouseY <= resY + resH && !filteredResolutions.empty()) {
        const auto resolutionCount = filteredResolutions.size();
        const auto midX            = resX + resW / 2.F;

        if (mouseX < midX) {
            if (selectedResolutionIndex > 0) {
                selectedResolutionIndex--;
            }
        } else {
            if (selectedResolutionIndex + 1 < resolutionCount) {
                selectedResolutionIndex++;
            }
        }

        selectedItem = OptionMenuItem::Resolution;
        updateChangeStatus();

        return true;
    }

    const auto [fullX, fullY, fullW, fullH] =
        getNodeLayout(fullScreenNode, menuBackgroundNodeX, menuBackgroundNodeY);

    if (mouseX >= fullX && mouseX <= fullX + fullW && mouseY >= fullY &&
        mouseY <= fullY + fullH) {
        selectedItem = OptionMenuItem::FullScreen;
        Maze::get().toggleFullscreen();
        return true;
    }

    const auto [vsyncX, vsyncY, vsyncW, vsyncH] = getNodeLayout(
        verticalSyncNode, menuBackgroundNodeX, menuBackgroundNodeY);

    if (mouseX >= vsyncX && mouseX <= vsyncX + vsyncW && mouseY >= vsyncY &&
        mouseY <= vsyncY + vsyncH) {
        selectedItem = OptionMenuItem::VerticalSync;
        Maze::get().setVerticalSync(!Maze::get().hasVerticalSync());
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

    auto [rootNodeX, rootNodeY, rootNodeW, rootNodeH] =
        getNodeLayout(rootNode, 0.F, 0.F);
    auto [menuNodeX, menuNodeY, menuNodeW, menuNodeH] =
        getNodeLayout(menuNode, rootNodeX, rootNodeY);
    auto [menuBackgroundNodeX, menuBackgroundNodeY, menuBackgroundNodeW,
          menuBackgroundNodeH] =
        getNodeLayout(menuBackgroundNode, menuNodeX, menuNodeY);

    auto isOver = [&](YGNodeRef node) {
        const auto [x, y, w, h] =
            getNodeLayout(node, menuBackgroundNodeX, menuBackgroundNodeY);
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

    filterResolutions();

    return false;
}

void OptionLayer::filterResolutions() {
    assert(selectedAspectRatioIndex < aspectRatioFilters.size());
    const auto& filter       = aspectRatioFilters[selectedAspectRatioIndex];
    auto        isExactMatch = [&](const sponge::core::Resolution& res) {
        const auto g  = std::gcd(res.width, res.height);
        const auto fg = std::gcd(filter.numerator, filter.denominator);
        return (res.width / g == filter.numerator / fg) &&
               (res.height / g == filter.denominator / fg);
    };

    auto matchesFilter = [&](const sponge::core::Resolution& res) {
        if (filter.approximate) {
            const float ratio =
                static_cast<float>(res.width) / static_cast<float>(res.height);
            const float target = static_cast<float>(filter.numerator) /
                                 static_cast<float>(filter.denominator);
            return !isExactMatch(res) &&
                   (std::abs(ratio - target) / target <= 0.01F);
        }
        return isExactMatch(res);
    };

    filteredResolutions = availableResolutions |
                          std::views::filter(matchesFilter) |
                          std::ranges::to<std::vector>();

    const auto window        = Maze::get().getWindow();
    const auto currentWidth  = window->getWidth();
    const auto currentHeight = window->getHeight();

    const auto it =
        std::ranges::find_if(filteredResolutions, [&](const auto& r) {
            return r.width == currentWidth && r.height == currentHeight;
        });
    selectedResolutionIndex = it != filteredResolutions.end() ?
                                  static_cast<size_t>(std::ranges::distance(
                                      filteredResolutions.begin(), it)) :
                                  0;

    updateChangeStatus();
}

void OptionLayer::updateChangeStatus() {
    if (filteredResolutions.empty()) {
        hasUnappliedChanges = false;
        returnButton->setMessage(std::string(returnMessage));
        return;
    }

    assert(selectedResolutionIndex < filteredResolutions.size());
    const auto  window = Maze::get().getWindow();
    const auto& res    = filteredResolutions[selectedResolutionIndex];
    hasUnappliedChanges =
        (res.width != window->getWidth() || res.height != window->getHeight());

    const auto expectedMessage =
        hasUnappliedChanges ? applyMessage : returnMessage;
    returnButton->setMessage(std::string(expectedMessage));
}

void OptionLayer::clearHoveredItems() {
    returnButton->setHover(false);
    hoveredItem = std::nullopt;
}
}  // namespace game::layer
