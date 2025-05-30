#include "mazelayer.hpp"
#include "maze.hpp"
#include "resourcemanager.hpp"
#include "scene/pointlight.hpp"
#include "sponge.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>  // For ortho, perspective, lookAt
#include <array>
#include <memory>  // For std::make_unique

namespace {
constexpr auto keyboardSpeed = .075F;
constexpr auto mouseSpeed = .125F;

constexpr auto cameraPosition = glm::vec3(0.F, 3.5F, 6.5F);
constexpr auto spotlightPosition = glm::vec3(-2.F, 4.F, -1.F);

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
    GameObject{ .name = "floor",
                .path = "/models/floor/floor.obj",
                .scale = glm::vec3(1.F),
                .translation = glm::vec3(0.F, 0.002F, 0.F) },

    GameObject{ .name = "cube1",
                .path = "/models/cube/cube-tex.obj",
                .scale = glm::vec3(1.F),
                .translation = glm::vec3(-1.5F, .85F, -.5F) },

    GameObject{ .name = "cube2",
                .path = "/models/cube/cube-tex.obj",
                .scale = glm::vec3(.5F),
                .translation = glm::vec3(0.F, 0.F, .5F) },

    GameObject{ .name = "cube3",
                .path = "/models/cube/cube-tex.obj",
                .scale = glm::vec3(.25F),
                .rotation = { .angle = glm::radians(60.F),
                              .axis = glm::vec3(1.F, 0.F, 1.F) },
                .translation = glm::vec3(-1.F, 0.25F, 1.F) }

    // GameObject{ .name = "helmet",
    //             .path = "/models/helmet/damaged_helmet.obj",
    //             .scale = glm::vec3(.5F),
    //             .rotation = { .angle = glm::radians(45.F),
    //                           .axis = glm::vec3(0.F, 1.F, 0.F) },
    //             .translation = glm::vec3(0.F, 0.F, 0.F) },

};

MazeLayer::MazeLayer() : Layer("maze") {
    // nothing
}
void MazeLayer::onAttach() {
    for (const auto& gameObject : gameObjects) {
        ResourceManager::loadModel(gameObject.name, gameObject.path);
    }

    camera = game::ResourceManager::createGameCamera(std::string(cameraName));
    camera->setViewportSize(Maze::get().getWidth(), Maze::get().getHeight());
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

    // shadowMap =
    // std::make_unique<sponge::platform::opengl::scene::ShadowMap>(); //
    // Removed

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
        if (!pointLights[i].shadowMap) {
            pointLights[i].shadowMap =
                std::make_unique<sponge::platform::opengl::scene::ShadowMap>();
        }

        pointLights[i].color = glm::vec3(1.F);
        pointLights[i].translation = glm::vec3(
            rotate(glm::mat4(1.F), glm::two_pi<float>() * i / numLights,
                   glm::vec3(0.F, 1.F, 0.F)) *
            glm::vec4(-1.F, 2.75F, -1.F, 1.F));
        // Update position based on translation for consistency
        // pointLights[i].position = pointLights[i].translation; // This will be
        // set by specific light configs

        if (i == 0) {  // Directional Light
            pointLights[i].type = LightType::DIRECTIONAL;
            pointLights[i].position = glm::vec3(
                0.f, 7.f,
                7.f);  // Position of the light volume/camera for ortho proj
            pointLights[i].direction =
                glm::normalize(glm::vec3(0.0f, -0.5f, -0.5f));
            pointLights[i].castsShadows = 1;
            pointLights[i].color = glm::vec3(0.8f);
            // fov and aspectRatio are not typically used for directional lights
            // in the same way, but can be set to default values if PointLight
            // struct requires them. The ortho projection matrix will define its
            // volume.
            pointLights[i].fov = glm::radians(45.0f);  // Default
            pointLights[i].aspectRatio = 1.0f;         // Default
        } else if (i == 1) {                           // Spot Light
            pointLights[i].type = LightType::SPOT;
            pointLights[i].position = glm::vec3(-3.f, 4.f, -3.f);
            pointLights[i].translation =
                pointLights[i].position;  // Keep translation consistent
            pointLights[i].direction =
                glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f));
            pointLights[i].fov = glm::radians(30.0f);
            pointLights[i].aspectRatio = 1.0f;
            pointLights[i].castsShadows = 1;
            pointLights[i].color = glm::vec3(1.0f, 1.0f, 0.8f);
        } else {  // Other lights (e.g., Point, not casting shadows for this
                  // test)
            pointLights[i].type = LightType::POINT;
            // Default position based on loop, or set explicitly if needed
            pointLights[i].position = glm::vec3(
                rotate(glm::mat4(1.F), glm::two_pi<float>() * i / numLights,
                       glm::vec3(0.F, 1.F, 0.F)) *
                glm::vec4(-1.F, 2.75F, -1.F, 1.F));
            pointLights[i].translation = pointLights[i].position;
            pointLights[i].direction =
                glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));  // Default
            pointLights[i].fov = glm::radians(90.0f);
            pointLights[i].aspectRatio = 1.0f;
            pointLights[i].castsShadows =
                0;  // Not casting shadows for this test
            pointLights[i].color = glm::vec3(0.5f);  // Dimmer color
        }

        // Common properties like attenuation can be set outside or after
        // specific configs
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
    constexpr float nearPlane = 1.0f;
    constexpr float farPlane = 25.0f;  // Adjusted far plane for potentially
                                       // larger scenes or ortho views

    auto shadowShader = ResourceManager::getShader(
        sponge::platform::opengl::scene::ShadowMap::getShaderName());

    // Shadow Pass: Render scene from each light's perspective
    glViewport(0, 0, sponge::platform::opengl::scene::ShadowMap::SHADOW_WIDTH,
               sponge::platform::opengl::scene::ShadowMap::
                   SHADOW_HEIGHT);  // Set viewport to shadow map size

    for (int i = 0; i < numLights; ++i) {
        PointLight& light = pointLights[i];
        if (!light.shadowMap)
            continue;

        light.shadowMap->bind();
        glClear(GL_DEPTH_BUFFER_BIT);  // Clear depth buffer for this light's
                                       // shadow map

        // Calculate View Matrix
        if (light.type == LightType::DIRECTIONAL) {
            // Directional light looks from its position towards position +
            // direction
            light.lightViewMatrix =
                glm::lookAt(light.position, light.position + light.direction,
                            glm::vec3(0.0f, 1.0f, 0.0f));
        } else {  // SPOT and POINT (simplified for now, point lights would
                  // ideally use cubemaps)
            light.lightViewMatrix =
                glm::lookAt(light.position, light.position + light.direction,
                            glm::vec3(0.0f, 1.0f, 0.0f));
        }

        // Calculate Projection Matrix
        if (light.type == LightType::DIRECTIONAL) {
            // Orthographic projection for directional lights
            light.lightProjectionMatrix =
                glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, nearPlane, farPlane);
        } else {  // SPOT and POINT (perspective projection)
            // Ensure FOV is reasonable, e.g. not zero, for perspective
            // projection
            float currentFov = (light.fov > glm::radians(0.01f))
                                   ? light.fov
                                   : glm::radians(45.0f);
            light.lightProjectionMatrix = glm::perspective(
                currentFov, light.aspectRatio, nearPlane, farPlane);
        }

        glm::mat4 currentLightSpaceMatrix =
            light.lightProjectionMatrix * light.lightViewMatrix;

        shadowShader->bind();
        shadowShader->setMat4("lightSpaceMatrix", currentLightSpaceMatrix);

        for (const auto& gameObject : gameObjects) {
            const auto model = glm::scale(
                glm::rotate(
                    glm::translate(glm::mat4(1.0f), gameObject.translation),
                    gameObject.rotation.angle, gameObject.rotation.axis),
                gameObject.scale);
            shadowShader->setMat4("model", model);
            ResourceManager::getModel(gameObject.name)->render(shadowShader);
        }
        light.shadowMap->unbind();
    }
    shadowShader->unbind();

    // Update shader light uniforms AFTER shadow maps have been generated and
    // matrices calculated
    updateShaderLights();

    // Main Scene Rendering Pass
    glViewport(0, 0, Maze::get().getWidth(), Maze::get().getHeight());
    glClear(GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);  // Clear default framebuffer

    auto pbrShader = ResourceManager::getShader(
        sponge::platform::opengl::scene::Mesh::getShaderName());
    pbrShader->bind();

    // Activate and Bind all necessary shadow map textures
    for (uint8_t i = 0; i < numLights; ++i) {
        if (pointLights[i].castsShadows == 1 && pointLights[i].shadowMap) {
            // Texture unit 0 is usually for diffuse. Shadow maps start from 1.
            // This texture unit (i + 1) must match what's set in
            // updateShaderLights for pointLights[i].shadowMapSampler
            pointLights[i].shadowMap->activateAndBindDepthMap(i + 1);
        }
    }

    pbrShader->setFloat3("viewPos", camera->getPosition());
    // Other PBR uniforms (metallic, roughness, ao, ambientStrength) are set in
    // onAttach or setters Light uniforms (including lightSpaceMatrix and
    // shadowMapSampler) are set by updateShaderLights()

    for (const auto& gameObject : gameObjects) {
        const auto model = glm::scale(
            glm::rotate(glm::translate(glm::mat4(1.0f), gameObject.translation),
                        gameObject.rotation.angle, gameObject.rotation.axis),
            gameObject.scale);
        pbrShader->setMat4("mvp", camera->getMVP() * model);
        pbrShader->setMat4("model", model);

        ResourceManager::getModel(gameObject.name)->render(pbrShader);
    }
    pbrShader->unbind();
}

void MazeLayer::renderLightCubes() const {
    const auto shader = ResourceManager::getShader(
        sponge::platform::opengl::scene::Cube::getShaderName());

    // render the spotlight as a cube
    shader->bind();
    shader->setFloat3("lightColor", glm::vec3(1.F, .87F, 0.F));
    shader->setMat4("mvp", scale(translate(camera->getMVP(), spotlightPosition),
                                 lightCubeScale));
    cube->render();
    shader->unbind();

    // render the point lights as cubes
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
    auto pbrShader = ResourceManager::getShader(
        sponge::platform::opengl::scene::Mesh::getShaderName());

    pbrShader->bind();
    pbrShader->setInteger("numLights", numLights);

    for (int32_t i = 0; i < numLights; ++i) {
        // Ensure position is updated if translation changes.
        // This was done in setNumLights, if lights are dynamic, this might need
        // to be updated each frame. pointLights[i].position =
        // glm::vec4(pointLights[i].translation, 1.F); // Already vec3, ensure
        // consistency. PointLight::position is already glm::vec3.

        pbrShader->setFloat3("pointLights[" + std::to_string(i) + "].position",
                             pointLights[i].position);
        pbrShader->setFloat3(
            "pointLights[" + std::to_string(i) + "].attenuation",
            pointLights[i].getAttenuation());
        pbrShader->setFloat3("pointLights[" + std::to_string(i) + "].color",
                             pointLights[i].color);

        // Calculate LightSpaceMatrix for the shader (using matrices computed in
        // shadow pass)
        glm::mat4 lightSpaceMatrixForShader =
            pointLights[i].lightProjectionMatrix *
            pointLights[i].lightViewMatrix;
        pbrShader->setMat4(
            "pointLights[" + std::to_string(i) + "].lightSpaceMatrix",
            lightSpaceMatrixForShader);

        // Texture units for shadow maps typically start from 1 (0 is often
        // diffuse) This needs to match the activateAndBindDepthMap call in
        // renderGameObjects
        pbrShader->setInteger(
            "pointLights[" + std::to_string(i) + "].shadowMapSampler", i + 1);
        pbrShader->setInteger(
            "pointLights[" + std::to_string(i) + "].castsShadows",
            pointLights[i].castsShadows);
    }

    pbrShader->unbind();
}
}  // namespace game::layer
