#include "mazelayer.hpp"
#include "resourcemanager.hpp"
#include "scene/pointlight.hpp"
#include "sponge.hpp"
#include <glm/ext/matrix_transform.hpp>

namespace {
constexpr auto keyboardSpeed = .075F;
constexpr auto mouseSpeed = .125F;

constexpr auto cameraPosition = glm::vec3(0.F, 2.5F, 6.5F);

constexpr auto lightCubeScale = glm::vec3(.1F);

constexpr char cameraName[] = "maze";

constexpr const char* colors[6] = { "FFFFFF", "FF1A1A", "1A1AFF",
                                    "1AFF1A", "FFFF1A", "1AFFFF" };

PointLight pointLights[6];
}  // namespace

namespace game::layer {

using sponge::input::KeyCode;
using sponge::platform::glfw::core::Application;
using sponge::platform::glfw::core::Input;
using sponge::platform::opengl::renderer::ResourceManager;

constexpr GameObject gameObjects[] = {
    { .name = const_cast<char*>("cube"),
      .path = const_cast<char*>("/models/cube/cube.obj"),
      .scale = glm::vec3(.5F),
      .translation = glm::vec3(-3.F, .5003F, 2.25F) },

    { .name = const_cast<char*>("texcube"),
      .path = const_cast<char*>("/models/cube/cube-tex.obj"),
      .scale = glm::vec3(.55F),
      .translation = glm::vec3(2.5F, .003F, 1.25F) },

    { .name = const_cast<char*>("vase"),
      .path = const_cast<char*>("/models/vase/flat_vase.obj"),
      .scale = glm::vec3(2.5F, 2.F, 2.5F),
      .translation = glm::vec3(-.25F, .003F, .25F) },

    { .name = const_cast<char*>("smoothVase"),
      .path = const_cast<char*>("/models/vase/smooth_vase.obj"),
      .scale = glm::vec3(2.5F),
      .translation = glm::vec3(.25F, .003F, .25F) },

    { .name = const_cast<char*>("sphere"),
      .path = const_cast<char*>("/models/sphere/flat_sphere.obj"),
      .scale = glm::vec3(.25F),
      .translation = glm::vec3(-2.25F, 1.F, 5.F) },

    { .name = const_cast<char*>("smoothSphere"),
      .path = const_cast<char*>("/models/sphere/smooth_sphere.obj"),
      .scale = glm::vec3(.25F),
      .translation = glm::vec3(2.8F, 1.F, 5.F) },

    { .name = const_cast<char*>("spider"),
      .path = const_cast<char*>("/models/spider/spider.obj"),
      .scale = glm::vec3(.8F),
      .translation = glm::vec3(2.5F, 0.003F, -0.25F) },

    { .name = const_cast<char*>("helmet"),
      .path = const_cast<char*>("/models/helmet/damaged_helmet.obj"),
      .scale = glm::vec3(.5F),
      .translation = glm::vec3(-2.15F, 0.F, 0.F) },

    { .name = const_cast<char*>("floor"),
      .path = const_cast<char*>("/models/floor/floor.obj"),
      .scale = glm::vec3(1.F),
      .translation = glm::vec3(0.F, 0.002F, 0.F) }
};

MazeLayer::MazeLayer() : Layer("maze") {
    // nothing
}

void MazeLayer::onAttach() {
    for (const auto& gameObject : gameObjects) {
        ResourceManager::loadModel(gameObject.name, gameObject.path);
    }

    camera = game::ResourceManager::createGameCamera(cameraName);
    camera->setPosition(cameraPosition);

    const auto shaderName =
        sponge::platform::opengl::scene::Mesh::getShaderName();
    const auto shader = ResourceManager::getShader(shaderName);
    shader->bind();

    shader->setFloat("metallic", metallic ? 1.F : 0.F);
    shader->setFloat("roughness", roughness);
    shader->setFloat("ao", ao);

    shader->setFloat("ambientStrength", ambientStrength);
    shader->unbind();

    cube = std::make_unique<sponge::platform::opengl::scene::Cube>();

    setNumLights(numLights);
}

void MazeLayer::onDetach() {
    // nothing
}

bool MazeLayer::onUpdate(const double elapsedTime) {
    updateShaderLights(elapsedTime);
    updateCamera(elapsedTime);

    renderGameObjects();
    renderLightCubes();

    return true;
}

void MazeLayer::setMetallic(const bool metallic) {
    this->metallic = metallic;

    const auto shader = ResourceManager::getShader(
        sponge::platform::opengl::scene::Mesh::getShaderName());
    shader->bind();
    shader->setFloat("metallic", metallic ? 1.F : 0.F);
    shader->unbind();
}

void MazeLayer::setAmbientOcclusion(const float ao) {
    this->ao = ao;

    const auto shader = ResourceManager::getShader(
        sponge::platform::opengl::scene::Mesh::getShaderName());
    shader->bind();
    shader->setFloat("ao", ao);
    shader->unbind();
}

void MazeLayer::setAmbientStrength(const float strength) {
    this->ambientStrength = strength;

    const auto shader = ResourceManager::getShader(
        sponge::platform::opengl::scene::Mesh::getShaderName());
    shader->bind();
    shader->setFloat("ambientStrength", ambientStrength);
    shader->unbind();
}

void MazeLayer::setRoughness(const float roughness) {
    this->roughness = roughness;

    const auto shader = ResourceManager::getShader(
        sponge::platform::opengl::scene::Mesh::getShaderName());
    shader->bind();
    shader->setFloat("roughness", roughness);
    shader->unbind();
}

void MazeLayer::setNumLights(const int32_t numLights) {
    this->numLights = numLights;

    for (int32_t i = 0; i < numLights; ++i) {
        pointLights[i].color = sponge::core::Color::hexToRGB(colors[i]);
        pointLights[i].translation = glm::vec3(
            rotate(glm::mat4(1.F), glm::two_pi<float>() * i / numLights,
                   glm::vec3(0.F, 1.F, 0.F)) *
            glm::vec4(-1.F, 1.5F, -1.F, 1.F));

        pointLights[i].setAttenuationFromIndex(attenuationIndex);
    }
}

void MazeLayer::setAttenuationIndex(const int32_t attenuationIndex) {
    this->attenuationIndex = attenuationIndex;
    setNumLights(numLights);
}

void MazeLayer::onEvent(sponge::event::Event& event) {
    sponge::event::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::event::MouseButtonPressedEvent>(
        BIND_EVENT_FN(onMouseButtonPressed));
    dispatcher.dispatch<sponge::event::MouseButtonReleasedEvent>(
        BIND_EVENT_FN(onMouseButtonReleased));
    dispatcher.dispatch<sponge::event::MouseScrolledEvent>(
        BIND_EVENT_FN(onMouseScrolled));
    dispatcher.dispatch<sponge::event::WindowResizeEvent>(
        BIND_EVENT_FN(onWindowResize));
}

bool MazeLayer::onMouseButtonPressed(
    const sponge::event::MouseButtonPressedEvent& event) {
    if (event.getMouseButton() == 0) {
        Application::get().centerMouse();
        Application::get().setMouseVisible(false);
        return true;
    }
    return false;
}

bool MazeLayer::onMouseButtonReleased(
    const sponge::event::MouseButtonReleasedEvent& event) {
    if (event.getMouseButton() == 0) {
        Application::get().setMouseVisible(true);
        return true;
    }
    return false;
}

bool MazeLayer::onMouseScrolled(
    const sponge::event::MouseScrolledEvent& event) const {
    camera->mouseScroll({ event.getXOffset(), event.getYOffset() });
    return true;
}

bool MazeLayer::onWindowResize(
    const sponge::event::WindowResizeEvent& event) const {
    camera->setViewportSize(event.getWidth(), event.getHeight());
    return false;
}

void MazeLayer::renderGameObjects() const {
    const auto shader = ResourceManager::getShader(
        sponge::platform::opengl::scene::Mesh::getShaderName());

    for (const auto& gameObject : gameObjects) {
        shader->bind();
        shader->setFloat3("viewPos", camera->getPosition());
        shader->setMat4("mvp",
                        translate(scale(camera->getMVP(), gameObject.scale),
                                  gameObject.translation));
        ResourceManager::getModel(gameObject.name)->render();
        shader->unbind();
    }
}

void MazeLayer::renderLightCubes() const {
    const auto shader = ResourceManager::getShader(
        sponge::platform::opengl::scene::Cube::getShaderName());

    for (auto i = 0; i < numLights; ++i) {
        shader->bind();
        shader->setFloat3("lightColor", pointLights[i].color);
        shader->setMat4(
            "mvp", scale(translate(camera->getMVP(), pointLights[i].position),
                         lightCubeScale));
        cube->render();
        shader->unbind();
    }
}

void MazeLayer::updateCamera(const double elapsedTime) const {
    if (Input::isKeyDown(KeyCode::SpongeKey_W) ||
        Input::isKeyDown(KeyCode::SpongeKey_Up)) {
        camera->moveForward(elapsedTime * keyboardSpeed);
    } else if (Input::isKeyDown(KeyCode::SpongeKey_S) ||
               Input::isKeyDown(KeyCode::SpongeKey_Down)) {
        camera->moveBackward(elapsedTime * keyboardSpeed);
    } else if (Input::isKeyDown(KeyCode::SpongeKey_A) ||
               Input::isKeyDown(KeyCode::SpongeKey_Left)) {
        camera->strafeLeft(elapsedTime * keyboardSpeed);
    } else if (Input::isKeyDown(KeyCode::SpongeKey_D) ||
               Input::isKeyDown(KeyCode::SpongeKey_Right)) {
        camera->strafeRight(elapsedTime * keyboardSpeed);
    }

    if (Input::isMouseButtonPressed(sponge::input::MouseButton::ButtonLeft)) {
        auto [xrel, yrel] = Input::getRelativeCursorPos();
        Input::setRelativeCursorPos({ 0.F, 0.F });
        camera->mouseMove({ xrel * mouseSpeed, yrel * mouseSpeed });
    }
}

void MazeLayer::updateShaderLights(const double elapsedTime) const {
    const auto shader = ResourceManager::getShader(
        sponge::platform::opengl::scene::Mesh::getShaderName());

    shader->bind();
    shader->setInteger("numLights", numLights);

    const auto rotateLight =
        rotate(glm::mat4(1.F), static_cast<float>(elapsedTime * 5),
               { 0.F, -1.F, 0.F });

    for (int32_t i = 0; i < numLights; ++i) {
        pointLights[i].translation =
            glm::vec3(rotateLight * glm::vec4(pointLights[i].translation, 1.F));
        pointLights[i].position = glm::vec4(pointLights[i].translation, 1.F);

        shader->setFloat3("pointLights[" + std::to_string(i) + "].position",
                          pointLights[i].position);
        shader->setFloat3("pointLights[" + std::to_string(i) + "].attenuation",
                          pointLights[i].getAttenuation());
        shader->setFloat3("pointLights[" + std::to_string(i) + "].color",
                          pointLights[i].color);
    }

    shader->unbind();
}

}  // namespace game::layer
