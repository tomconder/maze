#include "hudlayer.hpp"
#include "resourcemanager.hpp"

constexpr std::string_view cameraName = "hud";
constexpr std::string_view coffeeTexture = "coffee";
constexpr std::string_view quadShader = "quad";
constexpr std::string_view spriteShader = "sprite";

namespace game::layer {

HUDLayer::HUDLayer() : Layer("hud") {
    // nothing
}

void HUDLayer::onAttach() {
    sponge::platform::opengl::ResourceManager::loadShader(
        "/shaders/quad.vert", "/shaders/quad.frag", quadShader.data());

    sponge::platform::opengl::ResourceManager::loadShader(
        "/shaders/sprite.vert", "/shaders/sprite.frag", spriteShader.data());

    sponge::platform::opengl::ResourceManager::loadTexture(
        "/images/coffee.png", coffeeTexture.data());

    orthoCamera = ResourceManager::createOrthoCamera(cameraName.data());

    auto shader =
        sponge::platform::opengl::ResourceManager::getShader(quadShader.data());

    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    shader = sponge::platform::opengl::ResourceManager::getShader(
        spriteShader.data());

    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    logo = std::make_unique<sponge::platform::opengl::Sprite>(
        coffeeTexture.data());
}

void HUDLayer::onDetach() {
    // nothing
}

bool HUDLayer::onUpdate(const double elapsedTime) {
    UNUSED(elapsedTime);

    logo->render({ 12.F, 12.F }, { 64.F, 64.F });

    return true;
}

void HUDLayer::onEvent(sponge::event::Event& event) {
    sponge::event::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::event::WindowResizeEvent>(
        BIND_EVENT_FN(onWindowResize));
}

bool HUDLayer::onWindowResize(
    const sponge::event::WindowResizeEvent& event) const {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    const auto projection = orthoCamera->getProjection();
    auto shader = sponge::platform::opengl::ResourceManager::getShader(
        spriteShader.data());
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    shader =
        sponge::platform::opengl::ResourceManager::getShader(quadShader.data());
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    return false;
}

}  // namespace game::layer