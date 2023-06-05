#include "hudlayer.h"

void HUDLayer::onAttach() {
    orthoCamera = std::make_unique<sponge::OrthoCamera>();

    auto shader = sponge::OpenGLResourceManager::loadShader("assets/shaders/text.vert", "assets/shaders/text.frag", TEXT_SHADER);
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    font = sponge::OpenGLResourceManager::loadFont("assets/fonts/league-gothic/league-gothic.fnt", GOTHIC_FONT);

    shader = sponge::OpenGLResourceManager::loadShader("assets/shaders/sprite.vert", "assets/shaders/sprite.frag", SPRITE_SHADER);
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    sponge::OpenGLResourceManager::loadTexture("assets/images/coffee.png", COFFEE_TEXTURE);
    logo = std::make_unique<sponge::OpenGLSprite>(COFFEE_TEXTURE);
}

void HUDLayer::onDetach() {
    // nothing
}

bool HUDLayer::onUpdate(uint32_t elapsedTime) {
    UNUSED(elapsedTime);

    logo->render({ orthoCamera->getWidth() - 76.F, 12.F }, { 64.F, 64.F });
    font->render("Press [Q] to exit", { 12.F, orthoCamera->getHeight() - 12.F },
                 28, { 0.5, 0.9F, 1.0F });
    return true;
}

void HUDLayer::onEvent(sponge::Event& event) {
    sponge::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::WindowResizeEvent>(BIND_EVENT_FN(onWindowResize));
}

bool HUDLayer::onWindowResize(const sponge::WindowResizeEvent& event) {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    auto projection = orthoCamera->getProjection();
    auto shader = sponge::OpenGLResourceManager::getShader(SPRITE_SHADER);
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    shader = sponge::OpenGLResourceManager::getShader(TEXT_SHADER);
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    return false;
}
