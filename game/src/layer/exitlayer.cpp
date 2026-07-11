#include "layer/exitlayer.hpp"

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
constexpr std::string_view continueMessage     = "Continue";
constexpr std::string_view optionsMessage      = "Options";
constexpr std::string_view returnToMenuMessage = "Return to Menu";
constexpr std::string_view exitMessage         = "Exit the Game";

constexpr std::string_view cameraName = "exit";
constexpr std::string_view fontName   = "inter";
constexpr std::string_view fontPath   = "/fonts/inter.ttf";

constexpr glm::vec4 buttonColor    = { 0.05F, 0.05F, 0.05F, 1.F };
constexpr glm::vec3 textColor      = { 1.F, 1.F, 1.F };
constexpr glm::vec4 textHoverColor = { 0.84F, 0.04F, 0.04F, 0.07F };

std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> menuFont;

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

std::shared_ptr<game::scene::OrthoCamera> orthoCamera;

bool isRunning = true;
}  // namespace

namespace game::layer {
using sponge::event::Event;
using sponge::event::EventDispatcher;
using sponge::event::MouseButtonPressedEvent;
using sponge::event::MouseMovedEvent;
using sponge::event::WindowResizeEvent;
using sponge::input::GameAction;
using sponge::platform::glfw::core::Application;
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::FontCreateInfo;
using sponge::platform::opengl::scene::Quad;

ExitLayer::ExitLayer() : Layer("exit") {}

void ExitLayer::onAttach() {
    const auto fontCreateInfo = FontCreateInfo{ .name = std::string(fontName),
                                                .path = std::string(fontPath) };
    menuFont                  = AssetManager::createFont(fontCreateInfo);

    const auto orthoCameraCreateInfo =
        scene::OrthoCameraCreateInfo{ .name = std::string(cameraName) };
    orthoCamera = ResourceManager::createOrthoCamera(orthoCameraCreateInfo);

    auto makeMenuButton = [](std::string_view message) {
        return ui::makeMenuButton(
            message, ui::menuFontSizeForWidth(orthoCamera->getWidth()),
            menuFont, buttonColor, textColor);
    };

    continueButton     = makeMenuButton(continueMessage);
    optionsButton      = makeMenuButton(optionsMessage);
    returnToMenuButton = makeMenuButton(returnToMenuMessage);
    exitButton         = makeMenuButton(exitMessage);

    for (const auto& shader : { Quad::getShader(), menuFont->getShader() }) {
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    const auto skeleton = ui::buildMenuSkeleton(45.F);
    rootNode            = skeleton.root;
    menuNode            = skeleton.menu;
    menuBackgroundNode  = skeleton.menuBackground;

    continueNode     = ui::makeMenuRow(menuBackgroundNode, 0);
    optionsNode      = ui::makeMenuRow(menuBackgroundNode, 1);
    returnToMenuNode = ui::makeMenuRow(menuBackgroundNode, 2);
    exitNode         = ui::makeMenuRow(menuBackgroundNode, 3);

    const auto width  = static_cast<float>(orthoCamera->getWidth());
    const auto height = static_cast<float>(orthoCamera->getHeight());
    recalculateLayout(width, height);
}

void ExitLayer::onDetach() {
    YGNodeFreeRecursive(rootNode);
}

void ExitLayer::onEvent(Event& event) {
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

bool ExitLayer::onUpdate(const double elapsedTime) {
    {
        auto& mgr = Application::get().getInputManager();
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

        if (wasActiveLastFrame && !Maze::get().getOptionLayer()->isActive()) {
            const auto&    input     = mgr.getSnapshot();
            constexpr auto itemCount = static_cast<int>(ExitMenuItem::Count);

            if (input.isActive(GameAction::MenuDown)) {
                selectedItem = static_cast<ExitMenuItem>(
                    (static_cast<int>(selectedItem) + 1) % itemCount);
            }
            if (input.isActive(GameAction::MenuUp)) {
                selectedItem = static_cast<ExitMenuItem>(
                    (static_cast<int>(selectedItem) - 1 + itemCount) %
                    itemCount);
            }
            if (input.isActive(GameAction::MenuBack)) {
                mgr.consumeActive(GameAction::MenuBack);
                clearHoveredItems();
                selectedItem = ExitMenuItem::Continue;
                resumeGame();
            }
            if (!waitForConfirmRelease &&
                input.isActive(GameAction::MenuConfirm)) {
                mgr.consumeActive(GameAction::MenuConfirm);
                if (selectedItem == ExitMenuItem::Continue) {
                    resumeGame();
                } else if (selectedItem == ExitMenuItem::Options) {
                    clearHoveredItems();
                    Maze::get().getOptionLayer()->setActive(true);
                } else if (selectedItem == ExitMenuItem::ReturnToMenu) {
                    clearHoveredItems();
                    Maze::get().getIntroLayer()->setActive(true);
                    Maze::get().getMazeLayer()->setActive(false);
                    setActive(false);
                } else if (selectedItem == ExitMenuItem::Exit) {
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

    auto [rootNodeX, rootNodeY, rootNodeW, rootNodeH] =
        ui::getNodeLayout(rootNode, 0.F, 0.F);
    auto [menuNodeX, menuNodeY, menuNodeW, menuNodeH] =
        ui::getNodeLayout(menuNode, rootNodeX, rootNodeY);
    auto [menuBackgroundNodeX, menuBackgroundNodeY, menuBackgroundNodeW,
          menuBackgroundNodeH] =
        ui::getNodeLayout(menuBackgroundNode, menuNodeX, menuNodeY);

    const auto [continueX, continueY, continueW, continueH] = ui::getNodeLayout(
        continueNode, menuBackgroundNodeX, menuBackgroundNodeY);
    const auto [optionsX, optionsY, optionsW, optionsH] = ui::getNodeLayout(
        optionsNode, menuBackgroundNodeX, menuBackgroundNodeY);
    const auto [returnToMenuX, returnToMenuY, returnToMenuW, returnToMenuH] =
        ui::getNodeLayout(returnToMenuNode, menuBackgroundNodeX,
                          menuBackgroundNodeY);
    const auto [quitX, quitY, quitW, quitH] =
        ui::getNodeLayout(exitNode, menuBackgroundNodeX, menuBackgroundNodeY);

    continueButton->setPosition(
        { continueX, continueY },
        { continueX + continueW, continueY + continueH });
    optionsButton->setPosition({ optionsX, optionsY },
                               { optionsX + optionsW, optionsY + optionsH });
    returnToMenuButton->setPosition(
        { returnToMenuX, returnToMenuY },
        { returnToMenuX + returnToMenuW, returnToMenuY + returnToMenuH });
    exitButton->setPosition({ quitX, quitY }, { quitX + quitW, quitY + quitH });

    ui::updateMenuButtonVisuals(continueButton.get(),
                                selectedItem == ExitMenuItem::Continue,
                                textHoverColor);
    ui::updateMenuButtonVisuals(optionsButton.get(),
                                selectedItem == ExitMenuItem::Options,
                                textHoverColor);
    ui::updateMenuButtonVisuals(returnToMenuButton.get(),
                                selectedItem == ExitMenuItem::ReturnToMenu,
                                textHoverColor);
    ui::updateMenuButtonVisuals(
        exitButton.get(), selectedItem == ExitMenuItem::Exit, textHoverColor);

    UNUSED(continueButton->onUpdate(elapsedTime));
    UNUSED(optionsButton->onUpdate(elapsedTime));
    UNUSED(returnToMenuButton->onUpdate(elapsedTime));
    UNUSED(exitButton->onUpdate(elapsedTime));

    if (!isActive()) {
        wasActiveLastFrame    = false;
        waitForConfirmRelease = false;
        selectedItem          = ExitMenuItem::Continue;
    }

    return isRunning;
}

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

void ExitLayer::recalculateLayout(float width, float height) {
    const auto panelWidth = width * 0.54F;
    YGNodeStyleSetWidth(rootNode, panelWidth);
    YGNodeStyleSetHeight(rootNode, height);
    YGNodeCalculateLayout(rootNode, panelWidth, height, YGDirectionLTR);
}

bool ExitLayer::onMouseButtonPressed(const MouseButtonPressedEvent& event) {
    if (event.getMouseButton() != sponge::input::MouseButton::Button0) {
        return false;
    }

    auto [x, y] = Application::get().getMousePosition();
    if (continueButton->isInside({ x, y })) {
        clearHoveredItems();
        resumeGame();
    }

    if (optionsButton->isInside({ x, y })) {
        clearHoveredItems();
        Maze::get().getOptionLayer()->setActive(true);
    }

    if (returnToMenuButton->isInside({ x, y })) {
        clearHoveredItems();
        Maze::get().getIntroLayer()->setActive(true);
        Maze::get().getMazeLayer()->setActive(false);
        setActive(false);
    }

    if (exitButton->isInside({ x, y })) {
        isRunning = false;
    }

    return true;
}

bool ExitLayer::onMouseMoved(const MouseMovedEvent& event) {
    const auto pos = glm::vec2{ event.getX(), event.getY() };

    ui::updateButtonHover(continueButton.get(), pos);
    ui::updateButtonHover(optionsButton.get(), pos);
    ui::updateButtonHover(returnToMenuButton.get(), pos);
    ui::updateButtonHover(exitButton.get(), pos);

    return true;
}

void ExitLayer::resumeGame() {
    setActive(false);
#ifdef ENABLE_IMGUI
    if (Maze::get().getMazeLayer()->isImguiActive()) {
        Maze::get().getImGuiLayer()->setActive(true);
    }
#endif
}

void ExitLayer::clearHoveredItems() {
    continueButton->setHover(false);
    optionsButton->setHover(false);
    returnToMenuButton->setHover(false);
    exitButton->setHover(false);
}
}  // namespace game::layer
