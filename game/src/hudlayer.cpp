#include "hudlayer.h"

constexpr std::string_view coffeeTexture = "coffee";
constexpr std::string_view gothicFont = "league-gothic";
constexpr std::string_view quadShader = "quad";
constexpr std::string_view spriteShader = "sprite";
constexpr std::string_view textShader = "text";

void HUDLayer::onAttach() {
    auto assetsFolder = sponge::File::getResourceDir();

    orthoCamera = std::make_unique<sponge::OrthoCamera>();

    auto shader = sponge::OpenGLResourceManager::loadShader(
        assetsFolder + "/shaders/text.vert",
        assetsFolder + "/shaders/text.frag", textShader.data());
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    shader = sponge::OpenGLResourceManager::loadShader(
        assetsFolder + "/shaders/quad.vert",
        assetsFolder + "/shaders/quad.frag", quadShader.data());
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    font = sponge::OpenGLResourceManager::loadFont(
        assetsFolder + "/fonts/league-gothic/league-gothic.fnt",
        gothicFont.data());
    shader = sponge::OpenGLResourceManager::loadShader(
        assetsFolder + "/shaders/sprite.vert",
        assetsFolder + "/shaders/sprite.frag", spriteShader.data());
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    sponge::OpenGLResourceManager::loadTexture(
        assetsFolder + "/images/coffee.png", coffeeTexture.data());
    logo = std::make_unique<sponge::OpenGLSprite>(coffeeTexture.data());
}

void HUDLayer::onDetach() {
    // nothing
}

bool HUDLayer::onUpdate(uint32_t elapsedTime) {
    UNUSED(elapsedTime);

    logo->render({ orthoCamera->getWidth() - 76.F, 12.F }, { 64.F, 64.F });

    font->render("Maze", { 12.F, 12.F }, 32, { 0.05, 0.79F, 1.0F });
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
    auto shader = sponge::OpenGLResourceManager::getShader(spriteShader.data());
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    shader = sponge::OpenGLResourceManager::getShader(textShader.data());
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    return false;
}
