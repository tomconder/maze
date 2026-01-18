#include "layer/exitlayer.hpp"

#include "resourcemanager.hpp"
#include "scene/orthocamera.hpp"
#include "sponge.hpp"
#include "ui/button.hpp"

#include <yoga/Yoga.h>

#include <memory>
#include <string>

namespace {
constexpr std::string_view continueMessage     = "Continue";
constexpr std::string_view optionsMessage      = "Options";
constexpr std::string_view returnToMenuMessage = "Return to Menu";
constexpr std::string_view exitMessage         = "Exit the Game";

constexpr std::string_view cameraName = "exit";
constexpr std::string_view fontName   = "league-gothic";
constexpr std::string_view fontPath   = "/fonts/league-gothic.fnt";

constexpr glm::vec4 buttonColor    = { 0.05F, 0.05F, 0.05F, 1.F };
constexpr glm::vec4 hoverColor     = { 0.84F, 0.84F, 0.84F, 0.14F };
constexpr glm::vec3 textColor      = { 1.F, 1.F, 1.F };
constexpr glm::vec4 textHoverColor = { 0.84F, 0.04F, 0.04F, 0.07F };

inline std::string fontShaderName;
inline std::string quadShaderName;

std::unique_ptr<game::ui::Button> continueButton;
std::unique_ptr<game::ui::Button> optionsButton;
std::unique_ptr<game::ui::Button> returnToMenuButton;
std::unique_ptr<game::ui::Button> exitButton;

YGNodeRef continueNode       = nullptr;
YGNodeRef exitNode           = nullptr;
YGNodeRef menuBackgroundNode = nullptr;
YGNodeRef menuNode           = nullptr;
YGNodeRef optionsNode        = nullptr;
YGNodeRef returnToMenuNode   = nullptr;
YGNodeRef rootNode           = nullptr;
YGNodeRef titleNode          = nullptr;

std::shared_ptr<game::scene::OrthoCamera> orthoCamera;

bool isRunning = true;
}  // namespace

namespace game::layer {
using sponge::input::KeyCode;
using sponge::platform::glfw::core::Application;
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::FontCreateInfo;
using sponge::platform::opengl::scene::MSDFFont;
using sponge::platform::opengl::scene::Quad;

ExitLayer::ExitLayer() : Layer("exit") {
    fontShaderName = MSDFFont::getShaderName();
    quadShaderName = Quad::getShaderName();
}

void ExitLayer::onAttach() {
    const auto fontNameStr = std::string(fontName);

    const auto fontCreateInfo =
        FontCreateInfo{ .name = fontNameStr, .path = std::string(fontPath) };
    AssetManager::createFont(fontCreateInfo);

    const auto orthoCameraCreateInfo =
        scene::OrthoCameraCreateInfo{ .name = std::string(cameraName) };
    orthoCamera = ResourceManager::createOrthoCamera(orthoCameraCreateInfo);

    continueButton = std::make_unique<ui::Button>(
        ui::ButtonCreateInfo{ .topLeft      = glm::vec2{ 0.F },
                              .bottomRight  = glm::vec2{ 0.F },
                              .message      = std::string(continueMessage),
                              .fontSize     = 48,
                              .fontName     = fontNameStr,
                              .buttonColor  = buttonColor,
                              .textColor    = textColor,
                              .marginLeft   = 56,
                              .cornerRadius = 12.F,
                              .alignType = ui::ButtonAlignType::LeftAligned });

    optionsButton = std::make_unique<ui::Button>(
        ui::ButtonCreateInfo{ .topLeft      = glm::vec2{ 0.F },
                              .bottomRight  = glm::vec2{ 0.F },
                              .message      = std::string(optionsMessage),
                              .fontSize     = 48,
                              .fontName     = fontNameStr,
                              .buttonColor  = buttonColor,
                              .textColor    = textColor,
                              .marginLeft   = 56,
                              .cornerRadius = 12.F,
                              .alignType = ui::ButtonAlignType::LeftAligned });

    returnToMenuButton = std::make_unique<ui::Button>(
        ui::ButtonCreateInfo{ .topLeft      = glm::vec2{ 0.F },
                              .bottomRight  = glm::vec2{ 0.F },
                              .message      = std::string(returnToMenuMessage),
                              .fontSize     = 48,
                              .fontName     = fontNameStr,
                              .buttonColor  = buttonColor,
                              .textColor    = textColor,
                              .marginLeft   = 56,
                              .cornerRadius = 12.F,
                              .alignType = ui::ButtonAlignType::LeftAligned });
    exitButton = std::make_unique<ui::Button>(
        ui::ButtonCreateInfo{ .topLeft      = glm::vec2{ 0.F },
                              .bottomRight  = glm::vec2{ 0.F },
                              .message      = std::string(exitMessage),
                              .fontSize     = 48,
                              .fontName     = fontNameStr,
                              .buttonColor  = buttonColor,
                              .textColor    = textColor,
                              .marginLeft   = 56,
                              .cornerRadius = 12.F,
                              .alignType = ui::ButtonAlignType::LeftAligned });

    for (const auto& shaderName : { quadShaderName, fontShaderName }) {
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

    continueNode = YGNodeNew();
    YGNodeStyleSetFlex(continueNode, 1.0);
    YGNodeStyleSetMargin(continueNode, YGEdgeBottom, 5.F);
    YGNodeStyleSetMaxHeight(continueNode, 110);
    YGNodeInsertChild(menuBackgroundNode, continueNode, 0);

    optionsNode = YGNodeNew();
    YGNodeStyleSetFlex(optionsNode, 1.0);
    YGNodeStyleSetMargin(optionsNode, YGEdgeBottom, 5.F);
    YGNodeStyleSetMaxHeight(optionsNode, 110);
    YGNodeInsertChild(menuBackgroundNode, optionsNode, 1);

    returnToMenuNode = YGNodeNew();
    YGNodeStyleSetFlex(returnToMenuNode, 1.0);
    YGNodeStyleSetMargin(returnToMenuNode, YGEdgeBottom, 30.F);
    YGNodeStyleSetMaxHeight(returnToMenuNode, 110);
    YGNodeInsertChild(menuBackgroundNode, returnToMenuNode, 2);

    exitNode = YGNodeNew();
    YGNodeStyleSetFlex(exitNode, 1.0);
    YGNodeStyleSetMargin(exitNode, YGEdgeBottom, 30.F);
    YGNodeStyleSetMaxHeight(exitNode, 110);
    YGNodeInsertChild(menuBackgroundNode, exitNode, 3);

    auto [width, height] =
        std::pair{ static_cast<float>(orthoCamera->getWidth()),
                   static_cast<float>(orthoCamera->getHeight()) };
    recalculateLayout(width, height);
}

void ExitLayer::onDetach() {
    YGNodeFreeRecursive(rootNode);
}

void ExitLayer::onEvent(sponge::event::Event& event) {
    sponge::event::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::event::KeyPressedEvent>(
        [this](const sponge::event::KeyPressedEvent& event) {
            return isActive() ? this->onKeyPressed(event) : false;
        });
    dispatcher.dispatch<sponge::event::MouseButtonPressedEvent>(
        [this](const sponge::event::MouseButtonPressedEvent& event) {
            return isActive() ? this->onMouseButtonPressed(event) : false;
        });
    dispatcher.dispatch<sponge::event::MouseMovedEvent>(
        [this](const sponge::event::MouseMovedEvent& event) {
            return isActive() ? this->onMouseMoved(event) : false;
        });
    dispatcher.dispatch<sponge::event::MouseScrolledEvent>(
        [this](const sponge::event::MouseScrolledEvent& event) {
            return isActive() ? this->onMouseScrolled(event) : false;
        });
    dispatcher.dispatch<sponge::event::WindowResizeEvent>(
        [this](const sponge::event::WindowResizeEvent& event) {
            return this->onWindowResize(event);
        });
}

bool ExitLayer::onUpdate(const double elapsedTime) {
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

    const auto [continueX, continueY, continueW, continueH] =
        getNodeLayout(continueNode, menuBackgroundNodeX, menuBackgroundNodeY);
    const auto [optionsX, optionsY, optionsW, optionsH] =
        getNodeLayout(optionsNode, menuBackgroundNodeX, menuBackgroundNodeY);
    const auto [returnToMenuX, returnToMenuY, returnToMenuW, returnToMenuH] =
        getNodeLayout(returnToMenuNode, menuBackgroundNodeX,
                      menuBackgroundNodeY);
    const auto [quitX, quitY, quitW, quitH] =
        getNodeLayout(exitNode, menuBackgroundNodeX, menuBackgroundNodeY);

    continueButton->setPosition(
        { continueX, continueY },
        { continueX + continueW, continueY + continueH });
    optionsButton->setPosition({ optionsX, optionsY },
                               { optionsX + optionsW, optionsY + optionsH });
    returnToMenuButton->setPosition(
        { returnToMenuX, returnToMenuY },
        { returnToMenuX + returnToMenuW, returnToMenuY + returnToMenuH });
    exitButton->setPosition({ quitX, quitY }, { quitX + quitW, quitY + quitH });

    auto updateButtonVisuals = [this](ui::Button* button, ExitMenuItem item) {
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

    updateButtonVisuals(continueButton.get(), ExitMenuItem::Continue);
    updateButtonVisuals(optionsButton.get(), ExitMenuItem::Options);
    updateButtonVisuals(returnToMenuButton.get(), ExitMenuItem::ReturnToMenu);
    updateButtonVisuals(exitButton.get(), ExitMenuItem::Exit);

    UNUSED(continueButton->onUpdate(elapsedTime));
    UNUSED(optionsButton->onUpdate(elapsedTime));
    UNUSED(returnToMenuButton->onUpdate(elapsedTime));
    UNUSED(exitButton->onUpdate(elapsedTime));

    return isRunning;
}

bool ExitLayer::onWindowResize(
    const sponge::event::WindowResizeEvent& event) const {
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

void ExitLayer::recalculateLayout(float width, float height) const {
    const auto panelWidth = width * 0.54F;
    YGNodeStyleSetWidth(rootNode, panelWidth);
    YGNodeStyleSetHeight(rootNode, height);
    YGNodeCalculateLayout(rootNode, panelWidth, height, YGDirectionLTR);
}

bool ExitLayer::onKeyPressed(const sponge::event::KeyPressedEvent& event) {
    const auto     keyCode   = event.getKeyCode();
    constexpr auto itemCount = +ExitMenuItem::Count;

    if (keyCode == KeyCode::SpongeKey_Escape) {
        clearHoveredItems();
        selectedItem = ExitMenuItem::Continue;
        setActive(false);
    } else if (keyCode == KeyCode::SpongeKey_Enter ||
               keyCode == KeyCode::SpongeKey_KPEnter) {
        if (selectedItem == ExitMenuItem::Continue) {
            setActive(false);
        } else if (selectedItem == ExitMenuItem::Options) {
            optionsFlag = true;
        } else if (selectedItem == ExitMenuItem::Exit) {
            isRunning = false;
        }
    } else if (keyCode == KeyCode::SpongeKey_Down ||
               keyCode == KeyCode::SpongeKey_KP2) {
        selectedItem =
            static_cast<ExitMenuItem>((+selectedItem + 1) % itemCount);
    } else if (keyCode == KeyCode::SpongeKey_Up ||
               keyCode == KeyCode::SpongeKey_KP8) {
        selectedItem = static_cast<ExitMenuItem>(
            (+selectedItem - 1 + itemCount) % itemCount);
    }

    return true;
}

bool ExitLayer::onMouseButtonPressed(
    const sponge::event::MouseButtonPressedEvent& event) {
    UNUSED(event);

    auto [x, y] = sponge::platform::glfw::core::Input::getMousePosition();
    if (continueButton->isInside({ x, y })) {
        clearHoveredItems();
        setActive(false);
    }

    if (exitButton->isInside({ x, y })) {
        isRunning = false;
    }

    return true;
}

bool ExitLayer::onMouseMoved(
    const sponge::event::MouseMovedEvent& event) const {
    const auto pos = glm::vec2{ event.getX(), event.getY() };

    auto updateHover = [&pos](ui::Button* button) {
        if (!button->hasHover() && button->isInside(pos)) {
            button->setHover(true);
        } else if (button->hasHover() && !button->isInside(pos)) {
            button->setHover(false);
        }
    };

    updateHover(continueButton.get());
    updateHover(optionsButton.get());
    updateHover(returnToMenuButton.get());
    updateHover(exitButton.get());

    return true;
}

bool ExitLayer::onMouseScrolled(
    const sponge::event::MouseScrolledEvent& event) {
    UNUSED(event);
    return true;
}

void ExitLayer::clearHoveredItems() const {
    continueButton->setHover(false);
    optionsButton->setHover(false);
    returnToMenuButton->setHover(false);
    exitButton->setHover(false);
}
}  // namespace game::layer
