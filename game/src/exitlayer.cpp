#include "exitlayer.h"

static const char* const GOTHIC_FONT = "league-gothic";
static const char* const TEXT_SHADER = "text";

void ExitLayer::onAttach() {
    auto assetsFolder = sponge::File::getResourceDir();

    orthoCamera = std::make_unique<sponge::OrthoCamera>();

    font = sponge::OpenGLResourceManager::getFont(GOTHIC_FONT);
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

    font->render("Exit Game?",
                 { static_cast<float>(orthoCamera->getWidth()) / 2.F - 70.F,
                   static_cast<float>(orthoCamera->getHeight()) / 2.F + 136.F },
                 48, { 0.3F, 0.3F, 0.3F });

    font->render("Exit Game?",
                 { static_cast<float>(orthoCamera->getWidth()) / 2.F - 72.F,
                   static_cast<float>(orthoCamera->getHeight()) / 2.F + 140.F },
                 48, { 1.F, 1.F, 1.F });

    return true;
}

bool ExitLayer::onWindowResize(const sponge::WindowResizeEvent& event) {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    auto shader = sponge::OpenGLResourceManager::getShader(TEXT_SHADER);
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

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
