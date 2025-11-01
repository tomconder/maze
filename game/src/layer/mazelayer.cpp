#include "layer/mazelayer.hpp"

#include "maze.hpp"
#include "resourcemanager.hpp"
#include "scene/light.hpp"

#include <glm/ext/matrix_transform.hpp>

#include <array>
#include <memory>
#include <string>

namespace {
constexpr auto keyboardSpeed = .075F;
constexpr auto mouseSpeed = .125F;

constexpr auto cameraPosition = glm::vec3(0.F, 3.5F, 6.5F);

constexpr auto sunColor = glm::vec3(1.F, 1.F, 1.F);
constexpr auto sunDirection = glm::vec3(0.F, -2.F, 1.333F);
constexpr auto shadowMapRes = 2048;

constexpr auto cubeScale = glm::vec3(.1F);

constexpr std::string_view cameraName = "maze";

game::scene::DirectionalLight directionalLight;
std::array<game::scene::PointLight, 6> pointLights;
}  // namespace

namespace game::layer {
using sponge::input::KeyCode;
using sponge::platform::glfw::core::Application;
using sponge::platform::glfw::core::Input;
using sponge::platform::opengl::renderer::ResourceManager;
using sponge::platform::opengl::scene::Cube;
using sponge::platform::opengl::scene::Mesh;
using sponge::platform::opengl::scene::ShadowMap;

constexpr std::array gameObjects = {
    GameObject{ .name = "floor",
                .path = "/models/floor/floor.obj",
                .scale = glm::vec3(1.F),
                .translation = glm::vec3(0.F, 0.002F, 0.F) },

    // GameObject{ .name = "cube1",
    //             .path = "/models/cube/cube-tex.obj",
    //             .scale = glm::vec3(1.F),
    //             .translation = glm::vec3(-1.5F, .85F, -.5F) },
    //
    // GameObject{ .name = "cube2",
    //             .path = "/models/cube/cube-tex.obj",
    //             .scale = glm::vec3(.5F),
    //             .translation = glm::vec3(0.F, 0.F, .5F) },
    //
    // GameObject{ .name = "cube3",
    //             .path = "/models/cube/cube-tex.obj",
    //             .scale = glm::vec3(.25F),
    //             .rotation = { .angle = glm::radians(60.F),
    //                           .axis = glm::vec3(1.F, 0.F, 1.F) },
    //             .translation = glm::vec3(-1.F, 0.25F, 1.F) }

    GameObject{ .name = "helmet",
                .path = "/models/helmet/damaged_helmet.obj",
                .scale = glm::vec3(.5F),
                .rotation = { .angle = glm::radians(45.F),
                              .axis = glm::vec3(0.F, 1.F, 0.F) },
                .translation = glm::vec3(0.F, 0.F, 0.F) }
};

MazeLayer::MazeLayer() : Layer("maze") {
    // nothing
}

void MazeLayer::onAttach() {
    for (const auto& gameObject : gameObjects) {
        sponge::platform::opengl::scene::ModelCreateInfo modelCreateInfo{
            .name = gameObject.name, .path = gameObject.path
        };
        ResourceManager::createModel(modelCreateInfo);
    }

    const auto gameCameraCreateInfo =
        scene::GameCameraCreateInfo{ .name = std::string(cameraName) };
    camera = game::ResourceManager::createGameCamera(gameCameraCreateInfo);
    camera->setViewportSize(Maze::get().getWidth(), Maze::get().getHeight());
    camera->setPosition(cameraPosition);

    const auto shaderName = Mesh::getShaderName();
    const auto shader = ResourceManager::getShader(shaderName);
    shader->bind();

    shader->setFloat("metallic", metallic ? 1.F : 0.F);
    shader->setFloat("roughness", roughness);
    shader->setFloat("ao", ao);

    shader->setFloat("ambientStrength", ambientStrength);
    shader->setBoolean("directionalLightEnabled", directionalLightEnabled);
    shader->setBoolean("directionalLightCastsShadow",
                       directionalLightCastsShadow);
    shader->setFloat("shadowBias", shadowBias);
    shader->unbind();

    shadowMap = std::make_unique<ShadowMap>(shadowMapRes);
    cube = std::make_unique<Cube>();

    directionalLight = { .direction = sunDirection, .color = sunColor };

    setNumLights(numLights);
    updateShaderLights();
}

void MazeLayer::onDetach() {
    // nothing
}

bool MazeLayer::onUpdate(const double elapsedTime) {
    updateCamera(elapsedTime);

    renderSceneToDepthMap();
    renderGameObjects();
    renderLightCubes();

    return true;
}

void MazeLayer::setMetallic(const bool val) {
    metallic = val;

    const auto shader = ResourceManager::getShader(Mesh::getShaderName());
    shader->bind();
    shader->setFloat("metallic", metallic ? 1.F : 0.F);
    shader->unbind();
}

void MazeLayer::setAmbientOcclusion(const float val) {
    ao = val;

    const auto shader = ResourceManager::getShader(Mesh::getShaderName());
    shader->bind();
    shader->setFloat("ao", ao);
    shader->unbind();
}

void MazeLayer::setAmbientStrength(const float val) {
    ambientStrength = val;

    const auto shader = ResourceManager::getShader(Mesh::getShaderName());
    shader->bind();
    shader->setFloat("ambientStrength", ambientStrength);
    shader->unbind();
}

void MazeLayer::setRoughness(const float val) {
    roughness = val;

    const auto shader = ResourceManager::getShader(Mesh::getShaderName());
    shader->bind();
    shader->setFloat("roughness", roughness);
    shader->unbind();
}

void MazeLayer::setNumLights(const int32_t val) {
    numLights = val;

    for (int32_t i = 0; i < numLights; i++) {
        pointLights[i].color = glm::vec3(1.F);
        pointLights[i].translation = glm::vec3(
            rotate(glm::mat4(1.F), glm::two_pi<float>() * i / numLights,
                   glm::vec3(0.F, 1.F, 0.F)) *
            glm::vec4(0.F, 2.75F, -3.F, 1.F));

        const glm::vec4 attenuation =
            scene::Light::getAttenuationFromIndex(attenuationIndex);
        pointLights[i].distance = attenuation.x;
        pointLights[i].constant = attenuation.y;
        pointLights[i].linear = attenuation.z;
        pointLights[i].quadratic = attenuation.w;
    }

    updateShaderLights();
}

void MazeLayer::setAttenuationIndex(const int32_t val) {
    attenuationIndex = val;
    setNumLights(numLights);
}

glm::vec3 MazeLayer::getDirectionalLightColor() const {
    return directionalLight.color;
}

void MazeLayer::setDirectionalLightColor(const glm::vec3& color) {
    directionalLight.color = color;
    updateShaderLights();
}

glm::vec3 MazeLayer::getDirectionalLightDirection() const {
    return directionalLight.direction;
}

void MazeLayer::setDirectionalLightDirection(const glm::vec3& direction) {
    directionalLight.direction = direction;
    updateShaderLights();
}

void MazeLayer::setDirectionalLightEnabled(const bool value) {
    directionalLightEnabled = value;

    const auto shader = ResourceManager::getShader(Mesh::getShaderName());
    shader->bind();
    shader->setBoolean("directionalLightEnabled", directionalLightEnabled);
    shader->unbind();
}

void MazeLayer::setDirectionalLightCastsShadow(const bool value) {
    directionalLightCastsShadow = value;

    const auto shader = ResourceManager::getShader(Mesh::getShaderName());
    shader->bind();
    shader->setBoolean("directionalLightCastsShadow",
                       directionalLightCastsShadow);
    shader->unbind();
}

void MazeLayer::setShadowBias(const float value) {
    shadowBias = value;

    const auto shader = ResourceManager::getShader(Mesh::getShaderName());
    shader->bind();
    shader->setFloat("shadowBias", shadowBias);
    shader->unbind();
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
    glViewport(Maze::get().getOffsetX(), Maze::get().getOffsetY(),
               Maze::get().getWidth(), Maze::get().getHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto shader = ResourceManager::getShader(Mesh::getShaderName());

    for (const auto& gameObject : gameObjects) {
        shader->bind();
        shader->setFloat3("viewPos", camera->getPosition());
        const auto model = glm::scale(
            glm::rotate(glm::translate(glm::mat4(1.0f), gameObject.translation),
                        gameObject.rotation.angle, gameObject.rotation.axis),
            gameObject.scale);
        shader->setMat4("mvp", camera->getMVP() * model);
        shader->setMat4("model", model);
        shader->setMat4("lightSpaceMatrix", shadowMap->getLightSpaceMatrix());

        shader->setInteger("shadowMap", 1);
        shadowMap->activateAndBindDepthMap(1);

        ResourceManager::getModel(gameObject.name)->render(shader);
        shader->unbind();
    }
}

void MazeLayer::renderLightCubes() const {
    const auto shader = ResourceManager::getShader(Cube::getShaderName());

    // render the point lights as cubes
    for (auto i = 0; i < numLights; i++) {
        shader->bind();
        shader->setFloat3("lightColor", pointLights[i].color);
        shader->setMat4(
            "mvp", scale(translate(camera->getMVP(), pointLights[i].position),
                         cubeScale));
        cube->render();
        shader->unbind();
    }
}

void MazeLayer::renderSceneToDepthMap() const {
    shadowMap->bind();

    shadowMap->updateLightSpaceMatrix(
        glm::normalize(directionalLight.direction));
    const auto lightSpaceMatrix = shadowMap->getLightSpaceMatrix();

    auto shader = ResourceManager::getShader(ShadowMap::getShaderName());

    for (const auto& gameObject : gameObjects) {
        shader->bind();
        shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
        const auto model = glm::scale(
            glm::rotate(glm::translate(glm::mat4(1.0f), gameObject.translation),
                        gameObject.rotation.angle, gameObject.rotation.axis),
            gameObject.scale);
        shader->setMat4("model", model);

        ResourceManager::getModel(gameObject.name)->render(shader);
        shader->unbind();
    }

    shadowMap->unbind();
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
    const auto shader = ResourceManager::getShader(Mesh::getShaderName());

    shader->bind();

    // Set directional light
    shader->setFloat3("directionalLight.direction", directionalLight.direction);
    shader->setFloat3("directionalLight.color", directionalLight.color);

    // Set point lights
    shader->setInteger("numLights", numLights);

    for (int32_t i = 0; i < numLights; i++) {
        pointLights[i].position = glm::vec4(pointLights[i].translation, 1.F);

        shader->setFloat3("pointLights[" + std::to_string(i) + "].position",
                          pointLights[i].position);
        shader->setFloat3("pointLights[" + std::to_string(i) + "].color",
                          pointLights[i].color);
        shader->setFloat("pointLights[" + std::to_string(i) + "].constant",
                         pointLights[i].constant);
        shader->setFloat("pointLights[" + std::to_string(i) + "].linear",
                         pointLights[i].linear);
        shader->setFloat("pointLights[" + std::to_string(i) + "].quadratic",
                         pointLights[i].quadratic);
    }

    shader->unbind();
}
}  // namespace game::layer
