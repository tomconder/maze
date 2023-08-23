#include "exitlayer.h"

constexpr std::string_view gothicFont = "league-gothic";
constexpr std::string_view quadShader = "quad";

void ExitLayer::onAttach() {
    orthoCamera = std::make_unique<sponge::OrthoCamera>();

    auto shader = sponge::OpenGLResourceManager::getShader(quadShader.data());
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    font = sponge::OpenGLResourceManager::getFont(gothicFont.data());

    quad = std::make_unique<sponge::OpenGLQuad>();
}

void ExitLayer::onDetach() {
    // nothing
}

void ExitLayer::onEvent(sponge::Event& event) {
    sponge::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::KeyPressedEvent>(BIND_EVENT_FN(onKeyPressed));
    dispatcher.dispatch<sponge::MouseMovedEvent>(BIND_EVENT_FN(onMouseMoved));
    dispatcher.dispatch<sponge::MouseScrolledEvent>(
        BIND_EVENT_FN(onMouseScrolled));
    dispatcher.dispatch<sponge::WindowResizeEvent>(
        BIND_EVENT_FN(onWindowResize));
}

bool ExitLayer::onUpdate(uint32_t elapsedTime) {
    UNUSED(elapsedTime);

    auto width = static_cast<float>(orthoCamera->getWidth());
    auto height = static_cast<float>(orthoCamera->getHeight());

    quad->render({ 0.F, 0.F }, { width, height }, { 0.F, 0.F, 0.F, .84F });

    quad->render({ width * .23F, 0.F }, { width * .77F, height },
                 { .32F, .07F, .05F, .9F });

    std::string_view message = "Exit the Game?";
    uint32_t length = font->getLength(message, 48);
    font->render(message, { (width - length) / 2.F, height / 2.F - 128.F }, 48, { 1.F, 1.F, 1.F });

    quad->render({ width * .23F, height / 2.F - 36.F },
                 { width * .77F, height / 2.F + 72.F },
                 { .05F, .5F, .35F, 1.F });

    message = "Confirm";
    length = font->getLength(message, 52);
    font->render(message, { (width - length) / 2.F, height / 2.F - 2.F }, 52,
                 { 0.03F, 0.03F, 0.03F });

    quad->render({ width / 2.F - 132.F, height / 2.F + 117.F },
                 { width / 2.F + 132.F, height / 2.F + 186.F },
                 { .35F, .35F, .35F, 1.F });

    message = "Cancel";
    length = font->getLength(message, 36);
    font->render(message, { (width - length) / 2.F, height / 2.F + 135.F }, 36,
                 { 0.03F, 0.03F, 0.03F });

    return true;
}

void ExitLayer::setWidthAndHeight(uint32_t width, uint32_t height) {
    orthoCamera->setWidthAndHeight(width, height);

    auto shader = sponge::OpenGLResourceManager::getShader(quadShader.data());
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();
}

bool ExitLayer::onWindowResize(const sponge::WindowResizeEvent& event) {
    setWidthAndHeight(event.getWidth(), event.getHeight());
    return false;
}

bool ExitLayer::onKeyPressed(const sponge::KeyPressedEvent& event) {
    UNUSED(event);
    return true;
}

bool ExitLayer::onMouseMoved(const sponge::MouseMovedEvent& event) {
    UNUSED(event);
    return true;
}

bool ExitLayer::onMouseScrolled(const sponge::MouseScrolledEvent& event) {
    UNUSED(event);
    return true;
}
