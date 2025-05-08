#include "mazelayer.hpp"
#include "resourcemanager.hpp"
#include "scene/pointlight.hpp"
#include "sponge.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <array>

namespace {
constexpr auto keyboardSpeed = .075F;
constexpr auto mouseSpeed = .125F;

constexpr auto cameraPosition = glm::vec3(0.F, 2.5F, 6.5F);

constexpr auto lightCubeScale = glm::vec3(.1F);

constexpr std::string_view cameraName = "maze";

std::array<PointLight, 6> pointLights;
}  // namespace

namespace game::layer {
using sponge::input::KeyCode;
using sponge::platform::glfw::core::Application;
using sponge::platform::glfw::core::Input;
using sponge::platform::opengl::renderer::ResourceManager;

constexpr std::array gameObjects = {
    GameObject{ .name = "cube1",
                .path = "/models/cube/cube-tex.obj",
                .scale = glm::vec3(1.F),
                .translation = glm::vec3(-.5F, .85F, -.5F) },

    GameObject{ .name = "cube2",
                .path = "/models/cube/cube-tex.obj",
                .scale = glm::vec3(.5F),
                .translation = glm::vec3(1.5F, .25F, 1.F) },

    GameObject{ .name = "cube3",
                .path = "/models/cube/cube-tex.obj",
                .scale = glm::vec3(.25F),
                .rotation = { .angle = glm::radians(60.F),
                              .axis = glm::vec3(1.F, 0.F, 1.F) },
                .translation = glm::vec3(-1.F, 0.25F, 2.F) },

    // GameObject{ .name = "helmet",
    //             .path = "/models/helmet/damaged_helmet.obj",
    //             .scale = glm::vec3(.5F),
    //             .translation = glm::vec3(0.F, 0.F, 0.F) },

    GameObject{ .name = "floor",
                .path = "/models/floor/floor.obj",
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

    camera = game::ResourceManager::createGameCamera(std::string(cameraName));
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
    updateShaderLights();
}

void MazeLayer::onDetach() {
    // nothing
}

bool MazeLayer::onUpdate(const double elapsedTime) {
    updateCamera(elapsedTime);

    renderGameObjects();
    renderLightCubes();

    return true;
}

void MazeLayer::setMetallic(const bool val) {
    metallic = val;

    const auto shader = ResourceManager::getShader(
        sponge::platform::opengl::scene::Mesh::getShaderName());
    shader->bind();
    shader->setFloat("metallic", metallic ? 1.F : 0.F);
    shader->unbind();
}

void MazeLayer::setAmbientOcclusion(const float val) {
    ao = val;

    const auto shader = ResourceManager::getShader(
        sponge::platform::opengl::scene::Mesh::getShaderName());
    shader->bind();
    shader->setFloat("ao", ao);
    shader->unbind();
}

void MazeLayer::setAmbientStrength(const float val) {
    ambientStrength = val;

    const auto shader = ResourceManager::getShader(
        sponge::platform::opengl::scene::Mesh::getShaderName());
    shader->bind();
    shader->setFloat("ambientStrength", ambientStrength);
    shader->unbind();
}

void MazeLayer::setRoughness(const float val) {
    roughness = val;

    const auto shader = ResourceManager::getShader(
        sponge::platform::opengl::scene::Mesh::getShaderName());
    shader->bind();
    shader->setFloat("roughness", roughness);
    shader->unbind();
}

void MazeLayer::setNumLights(const int32_t val) {
    numLights = val;

    for (int32_t i = 0; i < numLights; ++i) {
        pointLights[i].color = glm::vec3(1.F);
        pointLights[i].translation = glm::vec3(
            rotate(glm::mat4(1.F), glm::two_pi<float>() * i / numLights,
                   glm::vec3(0.F, 1.F, 0.F)) *
            glm::vec4(-1.F, 1.5F, -1.F, 1.F));

        pointLights[i].setAttenuationFromIndex(attenuationIndex);
    }

    updateShaderLights();
}

void MazeLayer::setAttenuationIndex(const int32_t val) {
    attenuationIndex = val;
    setNumLights(numLights);
}

void MazeLayer::onEvent(sponge::event::Event& event) {
    sponge::event::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::event::MouseButtonPressedEvent>(
        [this](const sponge::event::MouseButtonPressedEvent& mbEvent) {
            return this->onMouseButtonPressed(mbEvent);
        });
    dispatcher.dispatch<sponge::event::MouseButtonReleasedEvent>(
        [this](const sponge::event::MouseButtonReleasedEvent& mrEvent) {
            return this->onMouseButtonReleased(mrEvent);
        });
    dispatcher.dispatch<sponge::event::MouseScrolledEvent>(
        [this](const sponge::event::MouseScrolledEvent& msEvent) {
            return this->onMouseScrolled(msEvent);
        });
    dispatcher.dispatch<sponge::event::WindowResizeEvent>(
        [this](const sponge::event::WindowResizeEvent& wsEvent) {
            return this->onWindowResize(wsEvent);
        });
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
        shader->setMat4(
            "mvp",
            scale(rotate(translate(camera->getMVP(), gameObject.translation),
                         gameObject.rotation.angle, gameObject.rotation.axis),
                  gameObject.scale));

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

void MazeLayer::updateShaderLights() const {
    const auto shader = ResourceManager::getShader(
        sponge::platform::opengl::scene::Mesh::getShaderName());

    shader->bind();
    shader->setInteger("numLights", numLights);

    for (int32_t i = 0; i < numLights; ++i) {
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
