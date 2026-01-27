#include "optionlayer.hpp"

#include "event/event.hpp"
#include "maze.hpp"
#include "resourcemanager.hpp"
#include "scene/orthocamera.hpp"
#include "sponge.hpp"
#include "ui/button.hpp"

#include <fmt/format.h>
#include <yoga/Yoga.h>

#include <memory>
#include <string>

namespace {
constexpr std::string_view returnMessage = "Return";

constexpr std::string_view cameraName = "intro";
constexpr std::string_view fontName   = "league-gothic";
constexpr std::string_view fontPath   = "/fonts/league-gothic.fnt";

constexpr glm::vec4 backgroundColor = { 0.F, 0.F, 0.F, 1.F };
constexpr glm::vec4 buttonColor     = { 0.F, 0.F, 0.F, 0.F };
constexpr glm::vec4 hoverColor      = { 0.84F, 0.84F, 0.84F, 0.14F };
constexpr glm::vec3 textColor       = { 1.F, 1.F, 1.F };
constexpr glm::vec4 textHoverColor  = { 0.84F, 0.04F, 0.04F, 0.14F };

inline std::string quadShaderName;
inline std::string fontShaderName;

std::unique_ptr<game::ui::Button> returnButton;

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

    resolutionNode = YGNodeNew();
    YGNodeStyleSetFlex(resolutionNode, 1.0);
    YGNodeStyleSetMargin(resolutionNode, YGEdgeBottom, 30.F);
    YGNodeStyleSetMaxHeight(resolutionNode, 110);
    YGNodeInsertChild(menuBackgroundNode, resolutionNode, 0);

    fullScreenNode = YGNodeNew();
    YGNodeStyleSetFlex(fullScreenNode, 1.0);
    YGNodeStyleSetMargin(fullScreenNode, YGEdgeBottom, 30.F);
    YGNodeStyleSetMaxHeight(fullScreenNode, 110);
    YGNodeInsertChild(menuBackgroundNode, fullScreenNode, 1);

    verticalSyncNode = YGNodeNew();
    YGNodeStyleSetFlex(verticalSyncNode, 1.0);
    YGNodeStyleSetMargin(verticalSyncNode, YGEdgeBottom, 30.F);
    YGNodeStyleSetMaxHeight(verticalSyncNode, 110);
    YGNodeInsertChild(menuBackgroundNode, verticalSyncNode, 2);

    returnNode = YGNodeNew();
    YGNodeStyleSetFlex(returnNode, 1.0);
    YGNodeStyleSetMargin(returnNode, YGEdgeBottom, 30.F);
    YGNodeStyleSetMaxHeight(returnNode, 110);
    YGNodeInsertChild(menuBackgroundNode, returnNode, 3);

    auto [width, height] =
        std::pair{ static_cast<float>(orthoCamera->getWidth()),
                   static_cast<float>(orthoCamera->getHeight()) };
    recalculateLayout(width, height);
}

void OptionLayer::onDetach() {
    YGNodeFreeRecursive(rootNode);
}

void OptionLayer::onEvent(Event& event) {
    EventDispatcher dispatcher(event);

    dispatcher.dispatch<KeyPressedEvent>([this](const KeyPressedEvent& event) {
        return isActive() ? this->onKeyPressed(event) : false;
    });
    dispatcher.dispatch<MouseButtonPressedEvent>(
        [this](const MouseButtonPressedEvent& event) {
            return isActive() ? this->onMouseButtonPressed(event) : false;
        });
    dispatcher.dispatch<MouseMovedEvent>([this](const MouseMovedEvent& event) {
        return isActive() ? this->onMouseMoved(event) : false;
    });
    dispatcher.dispatch<WindowResizeEvent>(
        [this](const WindowResizeEvent& event) {
            return this->onWindowResize(event);
        });
}

bool OptionLayer::onUpdate(double elapsedTime) {
    UNUSED(elapsedTime);

    const auto [width, height] =
        std::pair{ static_cast<float>(orthoCamera->getWidth()),
                   static_cast<float>(orthoCamera->getHeight()) };

    quad->render({ 0.F, 0.F }, { width, height }, backgroundColor);

    auto getNodeLayout = [](const YGNodeRef node, const float offsetX,
                            const float offsetY) {
        return std::tuple{ offsetX + YGNodeLayoutGetLeft(node),
                           offsetY + YGNodeLayoutGetTop(node),
                           YGNodeLayoutGetWidth(node),
                           YGNodeLayoutGetHeight(node) };
    };

    auto [rootNodeX, rootNodeY, rootNodeW, rootNodeH] =
        getNodeLayout(rootNode, 0.F, 0.F);
    auto [menuNodeX, menuNodeY, menuNodeW, menuNodeH] =
        getNodeLayout(menuNode, rootNodeX, rootNodeY);
    auto [menuBackgroundNodeX, menuBackgroundNodeY, menuBackgroundNodeW,
          menuBackgroundNodeH] =
        getNodeLayout(menuBackgroundNode, menuNodeX, menuNodeY);

    const auto [resolutionX, resolutionY, resolutionW, resolutionH] =
        getNodeLayout(resolutionNode, menuBackgroundNodeX, menuBackgroundNodeY);
    const auto [fullScreenX, fullScreenY, fullScreenW, fullScreenH] =
        getNodeLayout(fullScreenNode, menuBackgroundNodeX, menuBackgroundNodeY);
    const auto [verticalSyncX, verticalSyncY, verticalSyncW, verticalSyncH] =
        getNodeLayout(verticalSyncNode, menuBackgroundNodeX,
                      menuBackgroundNodeY);
    const auto [returnX, returnY, returnW, returnH] =
        getNodeLayout(returnNode, menuBackgroundNodeX, menuBackgroundNodeY);

    const auto fontNameStr = std::string(fontName);
    const auto font        = AssetManager::getFont(fontNameStr);

    const auto        window         = Maze::get().getWindow();
    const std::string exitMessageStr = fmt::format(
        "Resolution: {}x{}", window->getWidth(), window->getHeight());
    font->render(exitMessageStr, { resolutionX + 56.F, resolutionY }, 48,
                 { 1.F, 1.F, 1.F });

    const std::string fullScreenMessageStr = fmt::format(
        "Full Screen: {}", Maze::get().isFullscreen() ? "True" : "False");
    font->render(fullScreenMessageStr, { fullScreenX + 56.F, fullScreenY }, 48,
                 { 1.F, 1.F, 1.F });

    const std::string verticalSyncMessageStr = fmt::format(
        "Vertical Sync: {}", Maze::get().hasVerticalSync() ? "True" : "False");
    font->render(verticalSyncMessageStr,
                 { verticalSyncX + 56.F, verticalSyncY }, 48,
                 { 1.F, 1.F, 1.F });

    returnButton->setPosition({ returnX, returnY },
                              { returnX + returnW, returnY + returnH });

    auto updateButtonVisuals = [this](ui::Button* button, OptionMenuItem item) {
        if (selectedItem == item) {
            button->setBorderWidth(3.F);
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

    if (event.getKeyCode() == KeyCode::SpongeKey_Escape) {
        clearHoveredItems();
        setActive(false);
    }

    if (keyCode == KeyCode::SpongeKey_Enter ||
        keyCode == KeyCode::SpongeKey_KPEnter) {
        if (selectedItem == OptionMenuItem::Return) {
            clearHoveredItems();
            setActive(false);
        }
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

    return true;
}

bool OptionLayer::onMouseButtonPressed(const MouseButtonPressedEvent& event) {
    UNUSED(event);

    auto [x, y] = sponge::platform::glfw::core::Input::getMousePosition();

    if (returnButton->isInside({ x, y })) {
        clearHoveredItems();
        setActive(false);
    }

    return true;
}

bool OptionLayer::onMouseMoved(const MouseMovedEvent& event) const {
    const auto pos = glm::vec2{ event.getX(), event.getY() };

    auto updateHover = [&pos](ui::Button* button) {
        if (!button->hasHover() && button->isInside(pos)) {
            button->setHover(true);
        } else if (button->hasHover() && !button->isInside(pos)) {
            button->setHover(false);
        }
    };

    updateHover(returnButton.get());

    return true;
}

bool OptionLayer::onWindowResize(const WindowResizeEvent& event) const {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    for (const auto& shaderName : { fontShaderName, quadShaderName }) {
        const auto shader = AssetManager::getShader(shaderName);
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    const auto [width, height] =
        std::pair{ static_cast<float>(event.getWidth()),
                   static_cast<float>(event.getHeight()) };
    recalculateLayout(width, height);

    return false;
}

void OptionLayer::clearHoveredItems() const {
    // empty
}
}  // namespace game::layer
