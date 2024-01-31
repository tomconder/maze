#include "hudlayer.h"
#include "resourcemanager.h"

constexpr std::string_view cameraName = "hud";
constexpr std::string_view coffeeTexture = "coffee";
constexpr std::string_view quadShader = "quad";
constexpr std::string_view spriteShader = "sprite";

HUDLayer::HUDLayer() : Layer("hud") {
    // nothing
}

void HUDLayer::onAttach() {
    const auto assetsFolder = sponge::File::getResourceDir();

    const auto orthoCamera =
        ResourceManager::createOrthoCamera(cameraName.data());

    auto shader = sponge::graphics::renderer::OpenGLResourceManager::loadShader(
        assetsFolder + "/shaders/sprite.vert",
        assetsFolder + "/shaders/sprite.frag", spriteShader.data());
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    shader = sponge::graphics::renderer::OpenGLResourceManager::loadShader(
        assetsFolder + "/shaders/quad.vert",
        assetsFolder + "/shaders/quad.frag", quadShader.data());
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    sponge::graphics::renderer::OpenGLResourceManager::loadTexture(
        assetsFolder + "/images/coffee.png", coffeeTexture.data());
    logo = std::make_unique<sponge::graphics::renderer::OpenGLSprite>(
        coffeeTexture.data());
}

void HUDLayer::onDetach() {
    // nothing
}

bool HUDLayer::onUpdate(uint32_t elapsedTime, const bool isEventHandled) {
    UNUSED(elapsedTime);
    UNUSED(isEventHandled);

    const auto orthoCamera = ResourceManager::getOrthoCamera(cameraName.data());
    logo->render({ 12.F, 12.F }, { 64.F, 64.F });

    return true;
}

void HUDLayer::onEvent(sponge::event::Event& event) {
    sponge::event::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::event::WindowResizeEvent>(
        BIND_EVENT_FN(onWindowResize));
}

bool HUDLayer::onWindowResize(const sponge::event::WindowResizeEvent& event) {
    const auto orthoCamera = ResourceManager::getOrthoCamera(cameraName.data());
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    const auto projection = orthoCamera->getProjection();
    auto shader = sponge::graphics::renderer::OpenGLResourceManager::getShader(
        spriteShader.data());
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    shader = sponge::graphics::renderer::OpenGLResourceManager::getShader(
        quadShader.data());
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    return false;
}
