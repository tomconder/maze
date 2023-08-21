#include "hudlayer.h"

static const char* const COFFEE_TEXTURE = "coffee";
static const char* const GOTHIC_FONT = "league-gothic";
static const char* const SPRITE_SHADER = "sprite";
static const char* const TEXT_SHADER = "text";

void HUDLayer::onAttach() {
    auto assetsFolder = sponge::File::getResourceDir();

    orthoCamera = std::make_unique<sponge::OrthoCamera>();

    auto shader = sponge::OpenGLResourceManager::loadShader(
        assetsFolder + "/shaders/text.vert",
        assetsFolder + "/shaders/text.frag", TEXT_SHADER);
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    font = sponge::OpenGLResourceManager::loadFont(
        assetsFolder + "/fonts/league-gothic/league-gothic.fnt", GOTHIC_FONT);

    shader = sponge::OpenGLResourceManager::loadShader(
        assetsFolder + "/shaders/sprite.vert",
        assetsFolder + "/shaders/sprite.frag", SPRITE_SHADER);
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    sponge::OpenGLResourceManager::loadTexture(
        assetsFolder + "/images/coffee.png", COFFEE_TEXTURE);
    logo = std::make_unique<sponge::OpenGLSprite>(COFFEE_TEXTURE);
}

void HUDLayer::onDetach() {
    // nothing
}

bool HUDLayer::onUpdate(uint32_t elapsedTime) {
    UNUSED(elapsedTime);

    logo->render({ orthoCamera->getWidth() - 76.F, 12.F }, { 64.F, 64.F });

    font->render("Maze",
                 { 12.F, static_cast<float>(orthoCamera->getHeight()) - 12.F },
                 28, { 0.5, 0.9F, 1.0F });
    return true;
}

void HUDLayer::onEvent(sponge::Event& event) {
    sponge::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::WindowResizeEvent>(
        BIND_EVENT_FN(onWindowResize));
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
