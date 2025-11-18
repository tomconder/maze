#include "layer/exitlayer.hpp"

#include "resourcemanager.hpp"

#include <memory>
#include <string>

namespace {
inline const std::string cancelButtonMessage  = "Cancel";
inline const std::string confirmButtonMessage = "Confirm";
inline const std::string message              = "Exit the Game?";

inline const std::string cameraName = "exit";
inline const std::string fontName   = "league-gothic";
inline const std::string fontPath   = "/fonts/league-gothic.fnt";

constexpr glm::vec4 cancelButtonColor       = { .35F, .35F, .35F, 1.F };
constexpr glm::vec4 cancelButtonHoverColor  = { .63F, .63F, .63F, 1.F };
constexpr glm::vec4 confirmButtonColor      = { .05F, .5F, .35F, 1.F };
constexpr glm::vec4 confirmButtonHoverColor = { .13F, .65F, .53F, 1.F };
}  // namespace

namespace game::layer {
using sponge::platform::glfw::core::Application;
using sponge::platform::opengl::renderer::ResourceManager;
using sponge::platform::opengl::scene::Quad;

ExitLayer::ExitLayer() : Layer("exit") {
    // nothing
}

void ExitLayer::onAttach() {
    const auto fontCreateInfo =
        sponge::platform::opengl::scene::FontCreateInfo{ .name = fontName,
                                                         .path = fontPath };
    ResourceManager::createFont(fontCreateInfo);
    fontShaderName = sponge::platform::opengl::scene::Font::getShaderName();

    const auto orthoCameraCreateInfo =
        scene::OrthoCameraCreateInfo{ .name = cameraName };
    orthoCamera =
        game::ResourceManager::createOrthoCamera(orthoCameraCreateInfo);

    quad           = std::make_unique<Quad>();
    quadShaderName = Quad::getShaderName();

    confirmButton = std::make_unique<ui::Button>(
        glm::vec2{ 0.F }, glm::vec2{ 0.F }, confirmButtonMessage, 54, fontName,
        confirmButtonColor, glm::vec3{ 0.03F, 0.03F, 0.03F });

    cancelButton = std::make_unique<ui::Button>(
        glm::vec2{ 0.F }, glm::vec2{ 0.F }, cancelButtonMessage, 32, fontName,
        cancelButtonColor, glm::vec3{ 0.03F, 0.03F, 0.03F });

    auto shader = ResourceManager::getShader(quadShaderName);

    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    shader = ResourceManager::getShader(fontShaderName);

    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();
}

void ExitLayer::onDetach() {
    // nothing
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
    auto width  = static_cast<float>(orthoCamera->getWidth());
    auto height = static_cast<float>(orthoCamera->getHeight());

    quad->render({ 0.F, 0.F }, { width, height }, { 0.F, 0.F, 0.F, 0.56F });

    quad->render({ width * .23F, 0.F }, { width * .77F, height },
                 { .52F, .57F, .55F, 1.F });

    const auto font = ResourceManager::getFont(fontName);

    const uint32_t length = font->getLength(message, 48);
    font->render(
        message,
        { (width - static_cast<float>(length)) / 2.F, (height / 2.F) - 128.F },
        48, { 1.F, 1.F, 1.F });

    UNUSED(confirmButton->onUpdate(elapsedTime));
    UNUSED(cancelButton->onUpdate(elapsedTime));

    return isRunning;
}

bool ExitLayer::onWindowResize(
    const sponge::event::WindowResizeEvent& event) const {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    auto shader = ResourceManager::getShader(fontShaderName);
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    shader = ResourceManager::getShader(quadShaderName);
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    const auto inWidth  = static_cast<float>(event.getWidth());
    const auto inHeight = static_cast<float>(event.getHeight());

    confirmButton->setPosition({ inWidth * .23F, (inHeight / 2.F) - 30.F },
                               { inWidth * .77F, (inHeight / 2.F) + 78.F });

    cancelButton->setPosition(
        { (inWidth / 2.F) - 132.F, (inHeight / 2.F) + 117.F },
        { (inWidth / 2.F) + 132.F, (inHeight / 2.F) + 186.F });

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
    if (!cancelButton->hasHover() &&
        cancelButton->isInside({ event.getX(), event.getY() })) {
        cancelButton->setHover(true);
        cancelButton->setButtonColor(cancelButtonHoverColor);
    }

    if (cancelButton->hasHover() &&
        !cancelButton->isInside({ event.getX(), event.getY() })) {
        cancelButton->setHover(false);
        cancelButton->setButtonColor(cancelButtonColor);
    }

    if (!confirmButton->hasHover() &&
        confirmButton->isInside({ event.getX(), event.getY() })) {
        confirmButton->setHover(true);
        confirmButton->setButtonColor(confirmButtonHoverColor);
    }

    if (confirmButton->hasHover() &&
        !confirmButton->isInside({ event.getX(), event.getY() })) {
        confirmButton->setHover(false);
        confirmButton->setButtonColor(confirmButtonColor);
    }

    return true;
}

bool ExitLayer::onMouseScrolled(
    const sponge::event::MouseScrolledEvent& event) {
    UNUSED(event);
    return true;
}
}  // namespace game::layer
