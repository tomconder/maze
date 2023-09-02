#include "exitlayer.h"
#include "resourcemanager.h"

constexpr std::string_view cameraName = "hud";
constexpr std::string_view gothicFont = "league-gothic";
constexpr std::string_view quadShader = "quad";
constexpr glm::vec4 cancelButtonColor = { .35F, .35F, .35F, 1.F };
constexpr glm::vec4 cancelButtonHoverColor = { .63F, .63F, .63F, 1.F };
constexpr glm::vec4 confirmButtonColor = { .05F, .5F, .35F, 1.F };
constexpr glm::vec4 confirmButtonHoverColor = { .13F, .65F, .53F, 1.F };

void ExitLayer::onAttach() {
    auto orthoCamera = ResourceManager::createOrthoCamera(cameraName.data());

    auto shader = sponge::OpenGLResourceManager::getShader(quadShader.data());
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    auto assetsFolder = sponge::File::getResourceDir();
    auto font = sponge::OpenGLResourceManager::loadFont(
        assetsFolder + "/fonts/league-gothic/league-gothic.fnt",
        gothicFont.data());

    quad = std::make_unique<sponge::OpenGLQuad>();

    confirmButton = std::make_unique<ui::Button>(
        glm::vec2{ 0.F }, glm::vec2{ 0.F }, "Confirm", 54, confirmButtonColor,
        glm::vec3{ 0.03F, 0.03F, 0.03F });

    cancelButton = std::make_unique<ui::Button>(
        glm::vec2{ 0.F }, glm::vec2{ 0.F }, "Cancel", 32, cancelButtonColor,
        glm::vec3{ 0.03F, 0.03F, 0.03F });
}

void ExitLayer::onDetach() {
    // nothing
}

void ExitLayer::onEvent(sponge::Event& event) {
    sponge::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::KeyPressedEvent>(BIND_EVENT_FN(onKeyPressed));
    dispatcher.dispatch<sponge::MouseButtonPressedEvent>(
        BIND_EVENT_FN(onMouseClicked));
    dispatcher.dispatch<sponge::MouseMovedEvent>(BIND_EVENT_FN(onMouseMoved));
    dispatcher.dispatch<sponge::MouseScrolledEvent>(
        BIND_EVENT_FN(onMouseScrolled));
    dispatcher.dispatch<sponge::WindowResizeEvent>(
        BIND_EVENT_FN(onWindowResize));
}

bool ExitLayer::onUpdate(uint32_t elapsedTime) {
    auto orthoCamera = ResourceManager::getOrthoCamera(cameraName.data());
    auto width = static_cast<float>(orthoCamera->getWidth());
    auto height = static_cast<float>(orthoCamera->getHeight());

    quad->render({ 0.F, 0.F }, { width, height }, { 0.F, 0.F, 0.F, 1.F });

    quad->render({ width * .23F, 0.F }, { width * .77F, height },
                 { .52F, .57F, .55F, 1.F });

    auto font = sponge::OpenGLResourceManager::getFont(gothicFont.data());

    std::string_view message = "Exit the Game?";
    uint32_t length = font->getLength(message, 48);
    font->render(
        message,
        { (width - static_cast<float>(length)) / 2.F, height / 2.F - 128.F },
        48, { 1.F, 1.F, 1.F });

    confirmButton->onUpdate(elapsedTime);
    cancelButton->onUpdate(elapsedTime);

    return isRunning;
}

void ExitLayer::setWidthAndHeight(uint32_t width, uint32_t height) {
    auto orthoCamera = ResourceManager::getOrthoCamera(cameraName.data());
    orthoCamera->setWidthAndHeight(width, height);

    const auto shader =
        sponge::OpenGLResourceManager::getShader(quadShader.data());
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

bool ExitLayer::onWindowResize(const sponge::WindowResizeEvent& event) {
    setWidthAndHeight(event.getWidth(), event.getHeight());
    return false;
}

bool ExitLayer::onKeyPressed(const sponge::KeyPressedEvent& event) {
    UNUSED(event);
    return true;
}

bool ExitLayer::onMouseClicked(const sponge::MouseButtonPressedEvent& event) {
    if (cancelButton->isInside({ event.getX(), event.getY() })) {
        setActive(false);
    }

    if (confirmButton->isInside({ event.getX(), event.getY() })) {
        isRunning = false;
    }

    return false;
}

bool ExitLayer::onMouseMoved(const sponge::MouseMovedEvent& event) {
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

bool ExitLayer::onMouseScrolled(const sponge::MouseScrolledEvent& event) {
    UNUSED(event);
    return true;
}
