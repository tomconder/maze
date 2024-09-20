#include "hudlayer.hpp"
#include "resourcemanager.hpp"

namespace {
constexpr char cameraName[] = "hud";
constexpr char coffeeTexture[] = "coffee";
constexpr char quadShader[] = "quad";
constexpr char spriteShader[] = "sprite";
}  // namespace

namespace game::layer {

using sponge::platform::opengl::renderer::ResourceManager;

HUDLayer::HUDLayer() : Layer("hud") {
    // nothing
}

void HUDLayer::onAttach() {
    ResourceManager::loadShader("/shaders/quad.vert", "/shaders/quad.frag",
                                quadShader);

    ResourceManager::loadShader("/shaders/sprite.vert", "/shaders/sprite.frag",
                                spriteShader);

    ResourceManager::loadTexture("/textures/coffee.png", coffeeTexture);

    orthoCamera = game::ResourceManager::createOrthoCamera(cameraName);

    auto shader = ResourceManager::getShader(quadShader);

    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    shader = ResourceManager::getShader(spriteShader);

    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    logo = std::make_unique<sponge::platform::opengl::renderer::Sprite>(
        coffeeTexture);
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
    auto shader = ResourceManager::getShader(spriteShader);
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    shader = ResourceManager::getShader(quadShader);
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    return false;
}

}  // namespace game::layer
