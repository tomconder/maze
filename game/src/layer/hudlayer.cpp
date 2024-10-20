#include "hudlayer.hpp"
#include "resourcemanager.hpp"

namespace {
constexpr char cameraName[] = "hud";
constexpr char textureName[] = "coffee";
constexpr char texturePath[] = "/textures/coffee.png";
}  // namespace

namespace game::layer {

using sponge::platform::opengl::renderer::ResourceManager;

HUDLayer::HUDLayer() : Layer("hud") {
    // nothing
}

void HUDLayer::onAttach() {
    logo = std::make_unique<sponge::platform::opengl::scene::Sprite>(
        textureName, texturePath);

    orthoCamera = game::ResourceManager::createOrthoCamera(cameraName);

    shaderName = sponge::platform::opengl::scene::Sprite::getShaderName();
    const auto shader = ResourceManager::getShader(shaderName);

    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();
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
    const auto shader = ResourceManager::getShader(shaderName);
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    return false;
}

}  // namespace game::layer
