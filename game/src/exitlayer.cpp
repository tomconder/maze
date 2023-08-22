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

    quad->render({ 0.F, 0.F },
                 { orthoCamera->getWidth(), orthoCamera->getHeight() },
                 { 0.F, 0.F, 0.F, .7F });

    quad->render({ orthoCamera->getWidth() * .05F, 0.F },
                 { orthoCamera->getWidth() * .95F, orthoCamera->getHeight() },
                 { 0.F, 0.F, .1F, .7F });

    font->render("Exit Game?",
                 { static_cast<float>(orthoCamera->getWidth()) / 2.F - 90.F,
                   static_cast<float>(orthoCamera->getHeight()) / 2.F - 46.F },
                 48, { 0.3F, 0.3F, 0.3F });

    font->render("Exit Game?",
                 { static_cast<float>(orthoCamera->getWidth()) / 2.F - 92.F,
                   static_cast<float>(orthoCamera->getHeight()) / 2.F - 48.F },
                 48, { 1.F, 1.F, 1.F });

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
