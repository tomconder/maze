#include "hudlayer.h"
#include "resourcemanager.h"

constexpr std::string_view cameraName = "hud";
constexpr std::string_view coffeeTexture = "coffee";
constexpr std::string_view uiFont = "inter";
constexpr std::string_view quadShader = "quad";
constexpr std::string_view spriteShader = "sprite";
constexpr std::string_view textShader = "text";

HUDLayer::HUDLayer() : Layer("hud") {
    // nothing
}

void HUDLayer::onAttach() {
    const auto assetsFolder = sponge::File::getResourceDir();

    const auto orthoCamera =
        ResourceManager::createOrthoCamera(cameraName.data());

    auto shader = sponge::graphics::renderer::OpenGLResourceManager::loadShader(
        assetsFolder + "/shaders/text.vert",
        assetsFolder + "/shaders/text.frag", textShader.data());
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    shader = sponge::graphics::renderer::OpenGLResourceManager::loadShader(
        assetsFolder + "/shaders/quad.vert",
        assetsFolder + "/shaders/quad.frag", quadShader.data());
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    const auto font =
        sponge::graphics::renderer::OpenGLResourceManager::loadFont(
            assetsFolder + "/fonts/inter.fnt", uiFont.data());
    shader = sponge::graphics::renderer::OpenGLResourceManager::loadShader(
        assetsFolder + "/shaders/sprite.vert",
        assetsFolder + "/shaders/sprite.frag", spriteShader.data());
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

bool HUDLayer::onUpdate(uint32_t elapsedTime) {
    UNUSED(elapsedTime);

    const auto orthoCamera = ResourceManager::getOrthoCamera(cameraName.data());
    logo->render({ static_cast<float>(orthoCamera->getWidth()) - 76.F, 12.F },
                 { 64.F, 64.F });

    const auto font =
        sponge::graphics::renderer::OpenGLResourceManager::getFont(
            uiFont.data());
    font->render("Maze", { 12.F, 12.F }, 32, { .05F, .79F, 1.F });
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
        textShader.data());
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    return false;
}
