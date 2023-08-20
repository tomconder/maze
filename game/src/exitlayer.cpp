#include "exitlayer.h"

static const char* const GOTHIC_FONT = "league-gothic";
static const char* const TEXT_SHADER = "text";

void ExitLayer::onAttach() {
    auto assetsFolder = sponge::File::getResourceDir();

    orthoCamera = std::make_unique<sponge::OrthoCamera>();

    auto shader = sponge::OpenGLResourceManager::loadShader(
        assetsFolder + "/shaders/text.vert",
        assetsFolder + "/shaders/text.frag", TEXT_SHADER);

    font = sponge::OpenGLResourceManager::loadFont(
        assetsFolder + "/fonts/league-gothic/league-gothic.fnt", GOTHIC_FONT);

    SPONGE_INFO("ExitLayer attached");
}

void ExitLayer::onDetach() {
    // nothing
    SPONGE_INFO("ExitLayer detached");
}

void ExitLayer::onEvent(sponge::Event& event) {
    sponge::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::WindowResizeEvent>(
        BIND_EVENT_FN(onWindowResize));
}

bool ExitLayer::onUpdate(uint32_t elapsedTime) {
    UNUSED(elapsedTime);

    auto shader = sponge::OpenGLResourceManager::getShader(TEXT_SHADER);
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

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

void ExitLayer::setWidthAndHeight(uint32_t width, uint32_t height) {
    orthoCamera->setWidthAndHeight(width, height);

    auto shader = sponge::OpenGLResourceManager::getShader(TEXT_SHADER);
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();
}

bool ExitLayer::onWindowResize(const sponge::WindowResizeEvent& event) {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    auto shader = sponge::OpenGLResourceManager::getShader(TEXT_SHADER);
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    return false;
}
