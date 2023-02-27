#include "hudlayer.h"

void HUDLayer::onAttach() {
    orthoCamera = std::make_unique<Sponge::OrthoCamera>();

    auto shader =
        Sponge::OpenGLResourceManager::loadShader("assets/shaders/text.vert", "assets/shaders/text.frag", TEXT_SHADER);
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    font = Sponge::OpenGLResourceManager::loadFont("assets/fonts/league-gothic/league-gothic.fnt", GOTHIC_FONT);

    shader = Sponge::OpenGLResourceManager::loadShader("assets/shaders/sprite.vert", "assets/shaders/sprite.frag",
                                                       SPRITE_SHADER);
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    Sponge::OpenGLResourceManager::loadTexture("assets/images/coffee.png", COFFEE_TEXTURE);
    logo = std::make_unique<Sponge::OpenGLSprite>(COFFEE_TEXTURE);
}

void HUDLayer::onDetach() {
    // nothing
}

bool HUDLayer::onUpdate(uint32_t elapsedTime) {
    logo->render({ orthoCamera->getWidth() - 76.f, 12.f }, { 64.f, 64.f });
    font->render("Press [Q] to exit", { 12.f, orthoCamera->getHeight() - 12.f }, 28, { 0.5, 0.9f, 1.0f });
    return true;
}

void HUDLayer::onEvent(Sponge::Event& event) {
    Sponge::EventDispatcher dispatcher(event);

    dispatcher.dispatch<Sponge::WindowResizeEvent>(BIND_EVENT_FN(onWindowResize));
}

bool HUDLayer::onWindowResize(Sponge::WindowResizeEvent& event) {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    auto projection = orthoCamera->getProjection();
    auto shader = Sponge::OpenGLResourceManager::getShader(SPRITE_SHADER);
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    shader = Sponge::OpenGLResourceManager::getShader(TEXT_SHADER);
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    return false;
}
