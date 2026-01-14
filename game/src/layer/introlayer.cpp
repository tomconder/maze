#include "layer/introlayer.hpp"

#include "resourcemanager.hpp"
#include "yoga/Yoga.h"

#include <memory>
#include <string>

namespace {
constexpr std::string_view newGameMessage = "New Game";
constexpr std::string_view optionsMessage = "Options";
constexpr std::string_view quitMessage    = "Quit";

constexpr std::string_view cameraName = "intro";
constexpr std::string_view fontName   = "league-gothic";
constexpr std::string_view fontPath   = "/fonts/league-gothic.fnt";

constexpr glm::vec4 buttonColor = { 0.F, 0.F, 0.F, 0.F };
constexpr glm::vec3 textColor   = { 1.F, 1.F, 1.F };
constexpr glm::vec4 hoverColor  = { 0.84F, 0.84F, 0.84F, 0.14F };

inline std::string fontShaderName;
}  // namespace

namespace game::layer {
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::FontCreateInfo;
using sponge::platform::opengl::scene::MSDFFont;

IntroLayer::IntroLayer() : Layer("intro") {
    fontShaderName = MSDFFont::getShaderName();
}

void IntroLayer::onAttach() {
    const auto fontNameStr = std::string(fontName);

    const auto fontCreateInfo =
        FontCreateInfo{ .name = fontNameStr, .path = std::string(fontPath) };
    AssetManager::createFont(fontCreateInfo);

    const auto orthoCameraCreateInfo =
        scene::OrthoCameraCreateInfo{ .name = std::string(cameraName) };
    orthoCamera = ResourceManager::createOrthoCamera(orthoCameraCreateInfo);

    newGameButton = std::make_unique<ui::Button>(
        ui::ButtonCreateInfo{ .topLeft      = glm::vec2{ 0.F },
                              .bottomRight  = glm::vec2{ 0.F },
                              .message      = std::string(newGameMessage),
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

    quitButton = std::make_unique<ui::Button>(
        ui::ButtonCreateInfo{ .topLeft      = glm::vec2{ 0.F },
                              .bottomRight  = glm::vec2{ 0.F },
                              .message      = std::string(quitMessage),
                              .fontSize     = 48,
                              .fontName     = fontNameStr,
                              .buttonColor  = buttonColor,
                              .textColor    = textColor,
                              .marginLeft   = 56,
                              .cornerRadius = 12.F,
                              .alignType = ui::ButtonAlignType::LeftAligned });

    for (const auto& shaderName : { fontShaderName }) {
        const auto shader = AssetManager::getShader(shaderName);
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    rootNode = YGNodeNew();

    titleNode = YGNodeNew();
    YGNodeStyleSetFlexGrow(titleNode, 1.F);
    YGNodeInsertChild(rootNode, titleNode, 0);

    newGameNode = YGNodeNew();
    YGNodeStyleSetMinHeight(newGameNode, 80);
    YGNodeStyleSetMargin(newGameNode, YGEdgeBottom, 10.F);
    YGNodeInsertChild(rootNode, newGameNode, 1);

    optionsNode = YGNodeNew();
    YGNodeStyleSetMinHeight(optionsNode, 80);
    YGNodeStyleSetMargin(optionsNode, YGEdgeBottom, 10.F);
    YGNodeInsertChild(rootNode, optionsNode, 2);

    quitNode = YGNodeNew();
    YGNodeStyleSetMinHeight(quitNode, 80);
    YGNodeStyleSetMargin(quitNode, YGEdgeBottom, 50.F);
    YGNodeInsertChild(rootNode, quitNode, 3);

    auto [width, height] =
        std::pair{ static_cast<float>(orthoCamera->getWidth()),
                   static_cast<float>(orthoCamera->getHeight()) };
    recalculateLayout(width, height);
}

void IntroLayer::onDetach() {
    YGNodeFreeRecursive(rootNode);
}

void IntroLayer::onEvent(sponge::event::Event& event) {
    sponge::event::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::event::KeyPressedEvent>(
        [this](const sponge::event::KeyPressedEvent& event) {
            return this->onKeyPressed(event);
        });
    dispatcher.dispatch<sponge::event::MouseButtonPressedEvent>(
        [this](const sponge::event::MouseButtonPressedEvent& event) {
            return this->onMouseButtonPressed(event);
        });
    dispatcher.dispatch<sponge::event::MouseMovedEvent>(
        [this](const sponge::event::MouseMovedEvent& event) {
            return this->onMouseMoved(event);
        });
    dispatcher.dispatch<sponge::event::WindowResizeEvent>(
        [this](const sponge::event::WindowResizeEvent& event) {
            return this->onWindowResize(event);
        });
}

bool IntroLayer::onUpdate(const double elapsedTime) {
    const auto rootX = YGNodeLayoutGetLeft(rootNode);
    const auto rootY = YGNodeLayoutGetTop(rootNode);

    auto getNodeLayout = [rootX, rootY](const YGNodeRef node) {
        return std::tuple{ rootX + YGNodeLayoutGetLeft(node),
                           rootY + YGNodeLayoutGetTop(node),
                           YGNodeLayoutGetWidth(node),
                           YGNodeLayoutGetHeight(node) };
    };

    const auto [newGameX, newGameY, newGameW, newGameH] =
        getNodeLayout(newGameNode);
    const auto [optionsX, optionsY, optionsW, optionsH] =
        getNodeLayout(optionsNode);
    const auto [quitX, quitY, quitW, quitH] = getNodeLayout(quitNode);

    newGameButton->setPosition({ newGameX, newGameY },
                               { newGameX + newGameW, newGameY + newGameH });
    optionsButton->setPosition({ optionsX, optionsY },
                               { optionsX + optionsW, optionsY + optionsH });
    quitButton->setPosition({ quitX, quitY }, { quitX + quitW, quitY + quitH });

    UNUSED(newGameButton->onUpdate(elapsedTime));
    UNUSED(optionsButton->onUpdate(elapsedTime));
    UNUSED(quitButton->onUpdate(elapsedTime));

    return true;
}

bool IntroLayer::onWindowResize(
    const sponge::event::WindowResizeEvent& event) const {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    for (const auto& shaderName : { fontShaderName }) {
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

void IntroLayer::recalculateLayout(float width, float height) const {
    YGNodeStyleSetWidth(rootNode, width);
    YGNodeStyleSetHeight(rootNode, height);
    YGNodeCalculateLayout(rootNode, width, height, YGDirectionLTR);
}

bool IntroLayer::onKeyPressed(const sponge::event::KeyPressedEvent& event) {
    if (event.getKeyCode() == sponge::input::KeyCode::SpongeKey_Enter) {
        startGameFlag = true;
        return true;
    }

    if (event.getKeyCode() == sponge::input::KeyCode::SpongeKey_Escape) {
        quitFlag = true;
        return true;
    }

    return false;
}

bool IntroLayer::onMouseButtonPressed(
    const sponge::event::MouseButtonPressedEvent& event) {
    UNUSED(event);
    auto [x, y] = sponge::platform::glfw::core::Input::getMousePosition();

    if (newGameButton->isInside({ x, y })) {
        startGameFlag = true;
        return true;
    }

    if (quitButton->isInside({ x, y })) {
        quitFlag = true;
        return true;
    }

    return false;
}

bool IntroLayer::onMouseMoved(
    const sponge::event::MouseMovedEvent& event) const {
    const auto pos = glm::vec2{ event.getX(), event.getY() };

    auto updateHover = [&pos](ui::Button* button) {
        if (!button->hasHover() && button->isInside(pos)) {
            button->setHover(true);
            button->setButtonColor(hoverColor);
        } else if (button->hasHover() && !button->isInside(pos)) {
            button->setHover(false);
            button->setButtonColor(glm::vec4{ 0.F });
        }
    };

    updateHover(newGameButton.get());
    updateHover(optionsButton.get());
    updateHover(quitButton.get());

    return false;
}
}  // namespace game::layer
