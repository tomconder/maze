#include "layer/introlayer.hpp"

#include "maze.hpp"
#include "resourcemanager.hpp"
#include "scene/orthocamera.hpp"
#include "sponge.hpp"
#include "ui/button.hpp"

#include <yoga/Yoga.h>

#include <memory>
#include <string>

namespace {
constexpr std::string_view newGameMessage = "New Game";
constexpr std::string_view optionsMessage = "Options";
constexpr std::string_view quitMessage    = "Quit";

constexpr std::string_view cameraName = "intro";
constexpr std::string_view fontName   = "league-gothic";
constexpr std::string_view fontPath   = "/fonts/league-gothic.fnt";

constexpr glm::vec4 backgroundColor = { 0.12F, 0.19F, 0.29F, 1.F };
constexpr glm::vec4 buttonColor     = { 0.F, 0.F, 0.F, 0.F };
constexpr glm::vec4 hoverColor      = { 0.84F, 0.84F, 0.84F, 0.14F };
constexpr glm::vec3 textColor       = { 1.F, 1.F, 1.F };
constexpr glm::vec4 textHoverColor  = { 0.84F, 0.04F, 0.04F, 0.07F };

inline std::string fontShaderName;
inline std::string quadShaderName;

std::unique_ptr<game::ui::Button> newGameButton;
std::unique_ptr<game::ui::Button> optionsButton;
std::unique_ptr<game::ui::Button> quitButton;

YGNodeRef menuBackgroundNode = nullptr;
YGNodeRef menuNode           = nullptr;
YGNodeRef newGameNode        = nullptr;
YGNodeRef optionsNode        = nullptr;
YGNodeRef quitNode           = nullptr;
YGNodeRef rootNode           = nullptr;
YGNodeRef titleNode          = nullptr;

std::unique_ptr<sponge::platform::opengl::scene::Quad> quad;

std::shared_ptr<game::scene::OrthoCamera> orthoCamera;

bool isRunning = true;
}  // namespace

namespace game::layer {
using sponge::event::Event;
using sponge::event::EventDispatcher;
using sponge::event::KeyPressedEvent;
using sponge::event::KeyReleasedEvent;
using sponge::event::MouseButtonPressedEvent;
using sponge::event::MouseButtonReleasedEvent;
using sponge::event::MouseMovedEvent;
using sponge::event::MouseScrolledEvent;
using sponge::event::WindowResizeEvent;
using sponge::input::KeyCode;
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::FontCreateInfo;
using sponge::platform::opengl::scene::MSDFFont;
using sponge::platform::opengl::scene::Quad;

IntroLayer::IntroLayer() : Layer("intro") {
    fontShaderName = MSDFFont::getShaderName();
    quadShaderName = Quad::getShaderName();
}

void IntroLayer::onAttach() {
    const auto fontNameStr = std::string(fontName);

    const auto fontCreateInfo =
        FontCreateInfo{ .name = fontNameStr, .path = std::string(fontPath) };
    AssetManager::createFont(fontCreateInfo);

    const auto orthoCameraCreateInfo =
        scene::OrthoCameraCreateInfo{ .name = std::string(cameraName) };
    orthoCamera = ResourceManager::createOrthoCamera(orthoCameraCreateInfo);

    quad = std::make_unique<Quad>();

    auto makeMenuButton = [fontNameStr](std::string_view message) {
        return std::make_unique<ui::Button>(ui::ButtonCreateInfo{
            .topLeft      = glm::vec2{ 0.F },
            .bottomRight  = glm::vec2{ 0.F },
            .message      = std::string(message),
            .fontSize     = 48,
            .fontName     = fontNameStr,
            .buttonColor  = buttonColor,
            .textColor    = textColor,
            .marginLeft   = 26,
            .cornerRadius = 12.F,
            .alignType    = ui::ButtonAlignType::LeftAligned });
    };

    newGameButton = makeMenuButton(newGameMessage);
    optionsButton = makeMenuButton(optionsMessage);
    quitButton    = makeMenuButton(quitMessage);

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
    YGNodeStyleSetMargin(menuBackgroundNode, YGEdgeAll, 10.F);
    YGNodeStyleSetWidthPercent(menuBackgroundNode, 30.F);
    YGNodeInsertChild(menuNode, menuBackgroundNode, 0);

    auto makeMenuNode = [](const YGNodeRef parent, const int index) {
        auto* const child = YGNodeNew();
        YGNodeStyleSetFlex(child, 1.F);
        YGNodeStyleSetMaxHeight(child, 110);
        YGNodeInsertChild(parent, child, index);
        return child;
    };

    newGameNode = makeMenuNode(menuBackgroundNode, 0);
    optionsNode = makeMenuNode(menuBackgroundNode, 1);
    quitNode    = makeMenuNode(menuBackgroundNode, 2);

    const auto width  = static_cast<float>(orthoCamera->getWidth());
    const auto height = static_cast<float>(orthoCamera->getHeight());
    recalculateLayout(width, height);
}

void IntroLayer::onDetach() {
    YGNodeFreeRecursive(rootNode);
}

void IntroLayer::onEvent(Event& event) {
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

bool IntroLayer::onUpdate(const double elapsedTime) {
    for (const auto& shaderName : { fontShaderName, quadShaderName }) {
        const auto shader = AssetManager::getShader(shaderName);
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

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

    const auto [newGameX, newGameY, newGameW, newGameH] =
        getNodeLayout(newGameNode, menuBackgroundNodeX, menuBackgroundNodeY);
    const auto [optionsX, optionsY, optionsW, optionsH] =
        getNodeLayout(optionsNode, menuBackgroundNodeX, menuBackgroundNodeY);
    const auto [quitX, quitY, quitW, quitH] =
        getNodeLayout(quitNode, menuBackgroundNodeX, menuBackgroundNodeY);

    newGameButton->setPosition({ newGameX, newGameY },
                               { newGameX + newGameW, newGameY + newGameH });
    optionsButton->setPosition({ optionsX, optionsY },
                               { optionsX + optionsW, optionsY + optionsH });
    quitButton->setPosition({ quitX, quitY }, { quitX + quitW, quitY + quitH });

    auto updateButtonVisuals = [this](ui::Button* button, IntroMenuItem item) {
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

    updateButtonVisuals(newGameButton.get(), IntroMenuItem::NewGame);
    updateButtonVisuals(optionsButton.get(), IntroMenuItem::Options);
    updateButtonVisuals(quitButton.get(), IntroMenuItem::Quit);

    UNUSED(newGameButton->onUpdate(elapsedTime));
    UNUSED(optionsButton->onUpdate(elapsedTime));
    UNUSED(quitButton->onUpdate(elapsedTime));

    return isRunning;
}

bool IntroLayer::onWindowResize(const WindowResizeEvent& event) const {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    const auto [width, height] =
        std::pair{ static_cast<float>(event.getWidth()),
                   static_cast<float>(event.getHeight()) };
    recalculateLayout(width, height);

    return false;
}

void IntroLayer::recalculateLayout(float width, float height) {
    YGNodeStyleSetWidth(rootNode, width);
    YGNodeStyleSetHeight(rootNode, height);
    YGNodeCalculateLayout(rootNode, width, height, YGDirectionLTR);
}

bool IntroLayer::onKeyPressed(const KeyPressedEvent& event) {
    const auto     keyCode   = event.getKeyCode();
    constexpr auto itemCount = +IntroMenuItem::Count;

    if (keyCode == KeyCode::SpongeKey_Enter ||
        keyCode == KeyCode::SpongeKey_KPEnter) {
        if (selectedItem == IntroMenuItem::NewGame) {
            clearHoveredItems();
            setActive(false);
            Maze::get().getMazeLayer()->setActive(true);
#ifdef ENABLE_IMGUI
            if (Maze::get().getMazeLayer()->isImguiActive()) {
                Maze::get().getImGuiLayer()->setActive(true);
            }
#endif
            return true;
        }

        if (selectedItem == IntroMenuItem::Options) {
            clearHoveredItems();
            Maze::get().getOptionLayer()->setActive(true);
            return true;
        }

        if (selectedItem == IntroMenuItem::Quit) {
            isRunning = false;
            return true;
        }
    }

    if (keyCode == KeyCode::SpongeKey_Down ||
        keyCode == KeyCode::SpongeKey_KP2) {
        selectedItem =
            static_cast<IntroMenuItem>((+selectedItem + 1) % itemCount);
        return true;
    }

    if (keyCode == KeyCode::SpongeKey_Up || keyCode == KeyCode::SpongeKey_KP8) {
        selectedItem = static_cast<IntroMenuItem>(
            (+selectedItem - 1 + itemCount) % itemCount);
        return true;
    }

    return false;
}

bool IntroLayer::onMouseButtonPressed(const MouseButtonPressedEvent& event) {
    UNUSED(event);

    auto [x, y] = sponge::platform::glfw::core::Input::getMousePosition();

    if (newGameButton->isInside({ x, y })) {
        clearHoveredItems();
        setActive(false);
        Maze::get().getMazeLayer()->setActive(true);
#ifdef ENABLE_IMGUI
        if (Maze::get().getMazeLayer()->isImguiActive()) {
            Maze::get().getImGuiLayer()->setActive(true);
        }
#endif
        return true;
    }

    if (optionsButton->isInside({ x, y })) {
        clearHoveredItems();
        Maze::get().getOptionLayer()->setActive(true);
        return true;
    }

    if (quitButton->isInside({ x, y })) {
        isRunning = false;
        return true;
    }

    return false;
}

bool IntroLayer::onMouseMoved(const MouseMovedEvent& event) const {
    const auto pos = glm::vec2{ event.getX(), event.getY() };

    auto updateHover = [&pos](ui::Button* button) {
        if (!button->hasHover() && button->isInside(pos)) {
            button->setHover(true);
        } else if (button->hasHover() && !button->isInside(pos)) {
            button->setHover(false);
        }
    };

    updateHover(newGameButton.get());
    updateHover(optionsButton.get());
    updateHover(quitButton.get());

    return false;
}

void IntroLayer::clearHoveredItems() {
    newGameButton->setHover(false);
    optionsButton->setHover(false);
    quitButton->setHover(false);
}
}  // namespace game::layer
