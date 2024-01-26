#include "exitlayer.h"
#include "resourcemanager.h"

constexpr std::string_view cameraName = "hud";
constexpr std::string_view uiFont = "league-gothic";
constexpr std::string_view quadShader = "quad";
constexpr std::string_view textShader = "text";
constexpr glm::vec4 cancelButtonColor = { .35F, .35F, .35F, 1.F };
constexpr glm::vec4 cancelButtonHoverColor = { .63F, .63F, .63F, 1.F };
constexpr glm::vec4 confirmButtonColor = { .05F, .5F, .35F, 1.F };
constexpr glm::vec4 confirmButtonHoverColor = { .13F, .65F, .53F, 1.F };

ExitLayer::ExitLayer() : Layer("exit") {
    // nothing
}

void ExitLayer::onAttach() {
    const auto orthoCamera =
        ResourceManager::createOrthoCamera(cameraName.data());

    const auto assetsFolder = sponge::File::getResourceDir();

    auto shader = sponge::graphics::renderer::OpenGLResourceManager::loadShader(
        assetsFolder + "/shaders/text.vert",
        assetsFolder + "/shaders/text.frag", textShader.data());
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    shader = sponge::graphics::renderer::OpenGLResourceManager::getShader(
        quadShader.data());
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    sponge::graphics::renderer::OpenGLResourceManager::loadFont(
        assetsFolder + "/fonts/league-gothic.fnt", uiFont.data());

    quad = std::make_unique<sponge::graphics::renderer::OpenGLQuad>();

    confirmButton = std::make_unique<ui::Button>(
        glm::vec2{ 0.F }, glm::vec2{ 0.F }, confirmButtonMessage, 54, uiFont,
        confirmButtonColor, glm::vec3{ 0.03F, 0.03F, 0.03F });

    cancelButton = std::make_unique<ui::Button>(
        glm::vec2{ 0.F }, glm::vec2{ 0.F }, cancelButtonMessage, 32, uiFont,
        cancelButtonColor, glm::vec3{ 0.03F, 0.03F, 0.03F });
}

void ExitLayer::onDetach() {
    // nothing
}

void ExitLayer::onEvent(sponge::event::Event& event) {
    sponge::event::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::event::KeyPressedEvent>(
        BIND_EVENT_FN(onKeyPressed));
    dispatcher.dispatch<sponge::event::MouseButtonPressedEvent>(
        BIND_EVENT_FN(onMouseButtonPressed));
    dispatcher.dispatch<sponge::event::MouseMovedEvent>(
        BIND_EVENT_FN(onMouseMoved));
    dispatcher.dispatch<sponge::event::MouseScrolledEvent>(
        BIND_EVENT_FN(onMouseScrolled));
    dispatcher.dispatch<sponge::event::WindowResizeEvent>(
        BIND_EVENT_FN(onWindowResize));
}

bool ExitLayer::onUpdate(uint32_t elapsedTime) {
    const auto orthoCamera = ResourceManager::getOrthoCamera(cameraName.data());
    auto width = static_cast<float>(orthoCamera->getWidth());
    auto height = static_cast<float>(orthoCamera->getHeight());

    quad->render({ 0.F, 0.F }, { width, height }, { 0.F, 0.F, 0.F, 0.85F });

    quad->render({ width * .23F, 0.F }, { width * .77F, height },
                 { .52F, .57F, .55F, 1.F });

    const auto font =
        sponge::graphics::renderer::OpenGLResourceManager::getFont(
            uiFont.data());

    const uint32_t length = font->getLength(message, 48);
    font->render(
        message,
        { (width - static_cast<float>(length)) / 2.F, height / 2.F - 128.F },
        48, { 1.F, 1.F, 1.F });

    UNUSED(confirmButton->onUpdate(elapsedTime));
    UNUSED(cancelButton->onUpdate(elapsedTime));

    return isRunning;
}

void ExitLayer::setWidthAndHeight(uint32_t width, uint32_t height) const {
    const auto orthoCamera = ResourceManager::getOrthoCamera(cameraName.data());
    orthoCamera->setWidthAndHeight(width, height);

    auto shader = sponge::graphics::renderer::OpenGLResourceManager::getShader(
        textShader.data());
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    shader = sponge::graphics::renderer::OpenGLResourceManager::getShader(
        quadShader.data());
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    const auto inWidth = static_cast<float>(width);
    const auto inHeight = static_cast<float>(height);

    confirmButton->setPosition({ inWidth * .23F, inHeight / 2.F - 30.F },
                               { inWidth * .77F, inHeight / 2.F + 78.F });

    cancelButton->setPosition(
        { inWidth / 2.F - 132.F, inHeight / 2.F + 117.F },
        { inWidth / 2.F + 132.F, inHeight / 2.F + 186.F });
}

bool ExitLayer::onWindowResize(
    const sponge::event::WindowResizeEvent& event) const {
    setWidthAndHeight(event.getWidth(), event.getHeight());
    return false;
}

bool ExitLayer::onKeyPressed(
    const sponge::event::KeyPressedEvent& event) const {
    if (event.getKeyCode() == sponge::KeyCode::SpongeKey_Escape) {
        if (isActive()) {
            sponge::SDLEngine::get().setMouseVisible(false);
        } else {
            sponge::SDLEngine::get().setMouseVisible(true);
        }
    }

    return true;
}

bool ExitLayer::onMouseButtonPressed(
    const sponge::event::MouseButtonPressedEvent& event) {
    if (cancelButton->isInside({ event.getX(), event.getY() })) {
        setActive(false);
    }

    if (confirmButton->isInside({ event.getX(), event.getY() })) {
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
