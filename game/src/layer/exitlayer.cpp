#include "layer/exitlayer.hpp"

#include "resourcemanager.hpp"
#include "yoga/Yoga.h"

#include <memory>
#include <string>

namespace {
constexpr std::string_view cancelMessage  = "Cancel";
constexpr std::string_view confirmMessage = "Confirm";
constexpr std::string_view exitMessage    = "Exit the Game?";

constexpr std::string_view cameraName = "exit";
constexpr std::string_view fontName   = "league-gothic";
constexpr std::string_view fontPath   = "/fonts/league-gothic.fnt";

constexpr glm::vec4 cancelColor       = { .35F, .35F, .35F, 1.F };
constexpr glm::vec4 cancelHoverColor  = { .63F, .63F, .63F, 1.F };
constexpr glm::vec4 confirmColor      = { .05F, .5F, .35F, 1.F };
constexpr glm::vec4 confirmHoverColor = { .13F, .65F, .53F, 1.F };
constexpr glm::vec3 textColor         = { 0.03F, 0.03F, 0.03F };

inline std::string fontShaderName;
inline std::string quadShaderName;
}  // namespace

namespace game::layer {
using sponge::platform::glfw::core::Application;
using sponge::platform::opengl::renderer::ResourceManager;
using sponge::platform::opengl::scene::Font;
using sponge::platform::opengl::scene::FontCreateInfo;
using sponge::platform::opengl::scene::Quad;

ExitLayer::ExitLayer() : Layer("exit") {
    fontShaderName = Font::getShaderName();
    quadShaderName = Quad::getShaderName();
}

void ExitLayer::onAttach() {
    const auto fontCreateInfo = FontCreateInfo{ .name = std::string(fontName),
                                                .path = std::string(fontPath) };
    ResourceManager::createFont(fontCreateInfo);

    const auto orthoCameraCreateInfo =
        scene::OrthoCameraCreateInfo{ .name = std::string(cameraName) };
    orthoCamera =
        game::ResourceManager::createOrthoCamera(orthoCameraCreateInfo);

    quad = std::make_unique<Quad>();

    confirmButton = std::make_unique<ui::Button>(
        ui::ButtonCreateInfo{ .topLeft     = glm::vec2{ 0.F },
                              .bottomRight = glm::vec2{ 0.F },
                              .message     = std::string(confirmMessage),
                              .fontSize    = 54,
                              .fontName    = std::string(fontName),
                              .buttonColor = confirmColor,
                              .textColor   = textColor });

    cancelButton = std::make_unique<ui::Button>(
        ui::ButtonCreateInfo{ .topLeft      = glm::vec2{ 0.F },
                              .bottomRight  = glm::vec2{ 0.F },
                              .message      = std::string(cancelMessage),
                              .fontSize     = 32,
                              .fontName     = std::string(fontName),
                              .buttonColor  = cancelColor,
                              .textColor    = textColor,
                              .cornerRadius = 12.F });

    for (const auto& shaderName : { quadShaderName, fontShaderName }) {
        const auto shader = ResourceManager::getShader(shaderName);
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    rootNode = YGNodeNew();
    YGNodeStyleSetFlexDirection(rootNode, YGFlexDirectionColumn);
    YGNodeStyleSetJustifyContent(rootNode, YGJustifyCenter);
    YGNodeStyleSetAlignItems(rootNode, YGAlignStretch);

    messageNode = YGNodeNew();
    YGNodeStyleSetMargin(messageNode, YGEdgeBottom, 60.F);
    YGNodeStyleSetAlignSelf(messageNode, YGAlignCenter);
    YGNodeStyleSetPadding(messageNode, YGEdgeAll, 20.F);
    YGNodeInsertChild(rootNode, messageNode, 0);

    confirmNode = YGNodeNew();
    YGNodeStyleSetHeight(confirmNode, 100.F);
    YGNodeStyleSetWidthPercent(confirmNode, 100.F);
    YGNodeStyleSetMargin(confirmNode, YGEdgeBottom, 40.F);
    YGNodeInsertChild(rootNode, confirmNode, 1);

    cancelNode = YGNodeNew();
    YGNodeStyleSetHeight(cancelNode, 70.F);
    YGNodeStyleSetWidth(cancelNode, 280.F);
    YGNodeStyleSetAlignSelf(cancelNode, YGAlignCenter);
    YGNodeInsertChild(rootNode, cancelNode, 2);

    auto [width, height] =
        std::pair{ static_cast<float>(orthoCamera->getWidth()),
                   static_cast<float>(orthoCamera->getHeight()) };
    const auto panelWidth = width * 0.54F;
    YGNodeStyleSetWidth(rootNode, panelWidth);
    YGNodeStyleSetHeight(rootNode, height);
    YGNodeCalculateLayout(rootNode, panelWidth, height, YGDirectionLTR);
}

void ExitLayer::onDetach() {
    YGNodeFree(rootNode);
}

void ExitLayer::onEvent(sponge::event::Event& event) {
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
    dispatcher.dispatch<sponge::event::MouseScrolledEvent>(
        [this](const sponge::event::MouseScrolledEvent& event) {
            return this->onMouseScrolled(event);
        });
    dispatcher.dispatch<sponge::event::WindowResizeEvent>(
        [this](const sponge::event::WindowResizeEvent& event) {
            return this->onWindowResize(event);
        });
}

bool ExitLayer::onUpdate(const double elapsedTime) {
    auto [width, height] =
        std::pair{ static_cast<float>(orthoCamera->getWidth()),
                   static_cast<float>(orthoCamera->getHeight()) };

    quad->render({ 0.F, 0.F }, { width, height }, { 0.F, 0.F, 0.F, 0.56F });
    quad->render({ width * 0.23F, 0.F }, { width * 0.77F, height },
                 { 0.52F, 0.57F, 0.55F, 1.F });

    const float panelOffsetX = width * 0.23F;
    const auto  rootX        = panelOffsetX + YGNodeLayoutGetLeft(rootNode);
    const auto  rootY        = YGNodeLayoutGetTop(rootNode);

    auto getNodeLayout = [rootX, rootY](const YGNodeRef node) {
        return std::tuple{ rootX + YGNodeLayoutGetLeft(node),
                           rootY + YGNodeLayoutGetTop(node),
                           YGNodeLayoutGetWidth(node),
                           YGNodeLayoutGetHeight(node) };
    };

    const auto [confirmX, confirmY, confirmW, confirmH] =
        getNodeLayout(confirmNode);
    const auto [cancelX, cancelY, cancelW, cancelH] = getNodeLayout(cancelNode);
    const auto messageY = rootY + YGNodeLayoutGetTop(messageNode);

    const auto font         = ResourceManager::getFont(std::string(fontName));
    const auto length       = font->getLength(std::string(exitMessage), 48);
    const auto panelCenterX = width * 0.5F;
    font->render(std::string(exitMessage),
                 { panelCenterX - static_cast<float>(length) / 2.F, messageY },
                 48, { 1.F, 1.F, 1.F });

    confirmButton->setPosition({ confirmX, confirmY },
                               { confirmX + confirmW, confirmY + confirmH });
    cancelButton->setPosition({ cancelX, cancelY },
                              { cancelX + cancelW, cancelY + cancelH });

    UNUSED(confirmButton->onUpdate(elapsedTime));
    UNUSED(cancelButton->onUpdate(elapsedTime));

    return isRunning;
}

bool ExitLayer::onWindowResize(
    const sponge::event::WindowResizeEvent& event) const {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    for (const auto& shaderName : { fontShaderName, quadShaderName }) {
        const auto shader = ResourceManager::getShader(shaderName);
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    const auto [width, height] =
        std::pair{ static_cast<float>(event.getWidth()),
                   static_cast<float>(event.getHeight()) };
    const auto panelWidth = width * 0.54F;

    YGNodeStyleSetWidth(rootNode, panelWidth);
    YGNodeStyleSetHeight(rootNode, height);
    YGNodeCalculateLayout(rootNode, panelWidth, height, YGDirectionLTR);

    return false;
}

bool ExitLayer::onKeyPressed(
    const sponge::event::KeyPressedEvent& event) const {
    if (event.getKeyCode() == sponge::input::KeyCode::SpongeKey_Escape) {
        Application::get().setMouseVisible(!isActive());
    }

    return true;
}

bool ExitLayer::onMouseButtonPressed(
    const sponge::event::MouseButtonPressedEvent& event) {
    UNUSED(event);
    auto [x, y] = sponge::platform::glfw::core::Input::getMousePosition();
    if (cancelButton->isInside({ x, y })) {
        setActive(false);
    }

    if (confirmButton->isInside({ x, y })) {
        isRunning = false;
    }

    return true;
}

bool ExitLayer::onMouseMoved(
    const sponge::event::MouseMovedEvent& event) const {
    const auto pos = glm::vec2{ event.getX(), event.getY() };

    if (!cancelButton->hasHover() && cancelButton->isInside(pos)) {
        cancelButton->setHover(true);
        cancelButton->setButtonColor(cancelHoverColor);
    } else if (cancelButton->hasHover() && !cancelButton->isInside(pos)) {
        cancelButton->setHover(false);
        cancelButton->setButtonColor(cancelColor);
    }

    if (!confirmButton->hasHover() && confirmButton->isInside(pos)) {
        confirmButton->setHover(true);
        confirmButton->setButtonColor(confirmHoverColor);
    } else if (confirmButton->hasHover() && !confirmButton->isInside(pos)) {
        confirmButton->setHover(false);
        confirmButton->setButtonColor(confirmColor);
    }

    return true;
}

bool ExitLayer::onMouseScrolled(
    const sponge::event::MouseScrolledEvent& event) {
    UNUSED(event);
    return true;
}
}  // namespace game::layer
