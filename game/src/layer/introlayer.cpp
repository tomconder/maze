#include "layer/introlayer.hpp"

#include "maze.hpp"
#include "resourcemanager.hpp"
#include "scene/orthocamera.hpp"
#include "sponge.hpp"
#include "ui/button.hpp"
#include "ui/menufontsize.hpp"
#include "ui/menulayout.hpp"

#include <yoga/Yoga.h>

#include <memory>
#include <string>

namespace {
constexpr std::string_view newGameMessage = "New Game";
constexpr std::string_view optionsMessage = "Options";
constexpr std::string_view quitMessage    = "Quit";

constexpr std::string_view cameraName = "intro";
constexpr std::string_view fontName   = "inter";
constexpr std::string_view fontPath   = "/fonts/inter.ttf";

constexpr glm::vec4 backgroundColor = { 0.12F, 0.19F, 0.29F, 1.F };
constexpr glm::vec4 buttonColor     = { 0.F, 0.F, 0.F, 0.F };
constexpr glm::vec3 textColor       = { 1.F, 1.F, 1.F };
constexpr glm::vec4 textHoverColor  = { 0.84F, 0.04F, 0.04F, 0.07F };

std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> menuFont;

std::unique_ptr<game::ui::Button> newGameButton;
std::unique_ptr<game::ui::Button> optionsButton;
std::unique_ptr<game::ui::Button> quitButton;

YGNodeRef menuBackgroundNode = nullptr;
YGNodeRef menuNode           = nullptr;
YGNodeRef newGameNode        = nullptr;
YGNodeRef optionsNode        = nullptr;
YGNodeRef quitNode           = nullptr;
YGNodeRef rootNode           = nullptr;

std::unique_ptr<sponge::platform::opengl::scene::Quad> quad;

std::shared_ptr<game::scene::OrthoCamera> orthoCamera;

bool isRunning = true;
}  // namespace

namespace game::layer {
using sponge::event::Event;
using sponge::event::EventDispatcher;
using sponge::event::MouseButtonPressedEvent;
using sponge::event::MouseButtonReleasedEvent;
using sponge::event::MouseMovedEvent;
using sponge::event::MouseScrolledEvent;
using sponge::event::WindowResizeEvent;
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::FontCreateInfo;
using sponge::platform::opengl::scene::Quad;

IntroLayer::IntroLayer() : Layer("intro") {}

void IntroLayer::beginFadeIn(const double duration) {
    isFadingIn        = true;
    fadeInDuration    = duration;
    fadeInAccumulator = 0.0;
    setActive(true);
}

void IntroLayer::onAttach() {
    const auto fontCreateInfo = FontCreateInfo{ .name = std::string(fontName),
                                                .path = std::string(fontPath) };
    menuFont                  = AssetManager::createFont(fontCreateInfo);

    const auto orthoCameraCreateInfo =
        scene::OrthoCameraCreateInfo{ .name = std::string(cameraName) };
    orthoCamera = ResourceManager::createOrthoCamera(orthoCameraCreateInfo);

    quad = std::make_unique<Quad>();

    auto makeMenuButton = [](std::string_view message) {
        return ui::makeMenuButton(
            message, ui::menuFontSizeForWidth(orthoCamera->getWidth()),
            menuFont, buttonColor, textColor);
    };

    newGameButton = makeMenuButton(newGameMessage);
    optionsButton = makeMenuButton(optionsMessage);
    quitButton    = makeMenuButton(quitMessage);

    for (const auto& shader : { menuFont->getShader(), Quad::getShader() }) {
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    const auto skeleton = ui::buildMenuSkeleton(30.F);
    rootNode            = skeleton.root;
    menuNode            = skeleton.menu;
    menuBackgroundNode  = skeleton.menuBackground;

    newGameNode = ui::makeMenuRow(menuBackgroundNode, 0);
    optionsNode = ui::makeMenuRow(menuBackgroundNode, 1);
    quitNode    = ui::makeMenuRow(menuBackgroundNode, 2);

    const auto width  = static_cast<float>(orthoCamera->getWidth());
    const auto height = static_cast<float>(orthoCamera->getHeight());
    recalculateLayout(width, height);
}

void IntroLayer::onDetach() {
    YGNodeFreeRecursive(rootNode);
}

void IntroLayer::onEvent(Event& event) {
    EventDispatcher dispatcher(event);

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
    {
        auto& mgr =
            sponge::platform::glfw::core::Application::get().getInputManager();
        mgr.setActiveContext(sponge::input::InputContext::Menu);

        {
            using sponge::input::GameAction;
            const auto& input = mgr.getSnapshot();
            if (!wasActiveLastFrame) {
                waitForConfirmRelease = input.isHeld(GameAction::MenuConfirm);
            } else if (waitForConfirmRelease &&
                       !input.isHeld(GameAction::MenuConfirm)) {
                waitForConfirmRelease = false;
            }
        }

        if (wasActiveLastFrame && !isFadingIn &&
            !Maze::get().getOptionLayer()->isActive()) {
            using sponge::input::GameAction;
            const auto&    input     = mgr.getSnapshot();
            constexpr auto itemCount = static_cast<int>(IntroMenuItem::Count);

            if (input.isActive(GameAction::MenuDown)) {
                selectedItem = static_cast<IntroMenuItem>(
                    (static_cast<int>(selectedItem) + 1) % itemCount);
            }
            if (input.isActive(GameAction::MenuUp)) {
                selectedItem = static_cast<IntroMenuItem>(
                    (static_cast<int>(selectedItem) - 1 + itemCount) %
                    itemCount);
            }
            if (!waitForConfirmRelease &&
                input.isActive(GameAction::MenuConfirm)) {
                mgr.consumeActive(GameAction::MenuConfirm);
                if (selectedItem == IntroMenuItem::NewGame) {
                    clearHoveredItems();
                    setActive(false);
                    Maze::get().getMazeLayer()->setActive(true);
#ifdef ENABLE_IMGUI
                    if (Maze::get().getMazeLayer()->isImguiActive()) {
                        Maze::get().getImGuiLayer()->setActive(true);
                    }
#endif
                } else if (selectedItem == IntroMenuItem::Options) {
                    clearHoveredItems();
                    Maze::get().getOptionLayer()->setActive(true);
                } else if (selectedItem == IntroMenuItem::Quit) {
                    isRunning = false;
                }
            }
        }
        wasActiveLastFrame = true;
    }

    if (Maze::get().getOptionLayer()->isActive()) {
        return isRunning;
    }

    for (const auto& shader : { menuFont->getShader(), Quad::getShader() }) {
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    const auto [width, height] =
        std::pair{ static_cast<float>(orthoCamera->getWidth()),
                   static_cast<float>(orthoCamera->getHeight()) };

    quad->render({ 0.F, 0.F }, { width, height }, backgroundColor);

    auto [rootNodeX, rootNodeY, rootNodeW, rootNodeH] =
        ui::getNodeLayout(rootNode, 0.F, 0.F);
    auto [menuNodeX, menuNodeY, menuNodeW, menuNodeH] =
        ui::getNodeLayout(menuNode, rootNodeX, rootNodeY);
    auto [menuBackgroundNodeX, menuBackgroundNodeY, menuBackgroundNodeW,
          menuBackgroundNodeH] =
        ui::getNodeLayout(menuBackgroundNode, menuNodeX, menuNodeY);

    const auto [newGameX, newGameY, newGameW, newGameH] = ui::getNodeLayout(
        newGameNode, menuBackgroundNodeX, menuBackgroundNodeY);
    const auto [optionsX, optionsY, optionsW, optionsH] = ui::getNodeLayout(
        optionsNode, menuBackgroundNodeX, menuBackgroundNodeY);
    const auto [quitX, quitY, quitW, quitH] =
        ui::getNodeLayout(quitNode, menuBackgroundNodeX, menuBackgroundNodeY);

    newGameButton->setPosition({ newGameX, newGameY },
                               { newGameX + newGameW, newGameY + newGameH });
    optionsButton->setPosition({ optionsX, optionsY },
                               { optionsX + optionsW, optionsY + optionsH });
    quitButton->setPosition({ quitX, quitY }, { quitX + quitW, quitY + quitH });

    ui::updateMenuButtonVisuals(newGameButton.get(),
                                selectedItem == IntroMenuItem::NewGame,
                                textHoverColor);
    ui::updateMenuButtonVisuals(optionsButton.get(),
                                selectedItem == IntroMenuItem::Options,
                                textHoverColor);
    ui::updateMenuButtonVisuals(
        quitButton.get(), selectedItem == IntroMenuItem::Quit, textHoverColor);

    UNUSED(newGameButton->onUpdate(elapsedTime));
    UNUSED(optionsButton->onUpdate(elapsedTime));
    UNUSED(quitButton->onUpdate(elapsedTime));

    if (!isActive()) {
        wasActiveLastFrame    = false;
        waitForConfirmRelease = false;
        selectedItem          = IntroMenuItem::NewGame;
    }

    // Render gamepad connection status
    {
        const bool gamepadConnected =
            sponge::platform::glfw::core::Application::get()
                .getInputManager()
                .getSnapshot()
                .gamepadConnected;
        const auto gamepadStatus =
            gamepadConnected ? "Gamepad connected" : "No gamepad";
        constexpr auto statusFontSize = 16;
        constexpr auto margin         = 10.F;
        constexpr auto statusColor    = glm::vec3{ 0.6F, 0.6F, 0.6F };

        const auto statusWidth = static_cast<float>(
            menuFont->getLength(gamepadStatus, statusFontSize));
        menuFont->beginPass(statusFontSize);
        menuFont->render(
            gamepadStatus,
            { width - statusWidth - margin,
              height - menuFont->getHeight(statusFontSize) - margin },
            statusColor);
        menuFont->endPass();
    }

    if (isFadingIn) {
        fadeInAccumulator += elapsedTime;
        if (fadeInAccumulator >= fadeInDuration) {
            isFadingIn = false;
        }
    }

    return isRunning;
}

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

void IntroLayer::recalculateLayout(float width, float height) {
    YGNodeStyleSetWidth(rootNode, width);
    YGNodeStyleSetHeight(rootNode, height);
    YGNodeCalculateLayout(rootNode, width, height, YGDirectionLTR);
}

bool IntroLayer::onMouseButtonPressed(const MouseButtonPressedEvent& event) {
    if (isFadingIn ||
        event.getMouseButton() != sponge::input::MouseButton::Button0) {
        return false;
    }

    auto [x, y] =
        sponge::platform::glfw::core::Application::get().getMousePosition();

    if (newGameButton->isInside({ x, y })) {
        selectedItem = IntroMenuItem::NewGame;
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
        selectedItem = IntroMenuItem::Options;
        clearHoveredItems();
        Maze::get().getOptionLayer()->setActive(true);
        return true;
    }

    if (quitButton->isInside({ x, y })) {
        selectedItem = IntroMenuItem::Quit;
        isRunning    = false;
        return true;
    }

    return false;
}

bool IntroLayer::onMouseMoved(const MouseMovedEvent& event) {
    const auto pos = glm::vec2{ event.getX(), event.getY() };

    ui::updateButtonHover(newGameButton.get(), pos);
    ui::updateButtonHover(optionsButton.get(), pos);
    ui::updateButtonHover(quitButton.get(), pos);

    return false;
}

void IntroLayer::clearHoveredItems() {
    newGameButton->setHover(false);
    optionsButton->setHover(false);
    quitButton->setHover(false);
}
}  // namespace game::layer
