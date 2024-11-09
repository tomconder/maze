#include "mazelayer.hpp"
#include "resourcemanager.hpp"
#include <glm/ext/matrix_transform.hpp>

namespace {
constexpr auto keyboardSpeed = .1F;
constexpr auto mouseSpeed = .1F;

constexpr auto cameraPosition = glm::vec3(0.F, 2.5F, 6.5F);

constexpr auto lightCubeScale = glm::vec3(.1F);

constexpr auto lineColor = glm::vec3(.05F, .75F, 0.F);
constexpr auto lineWidth = .3F;

constexpr char cameraName[] = "maze";

glm::vec3 lightColors[6] = { { 1.F, 1.F, 1.F }, { 1.F, .1F, .1F },
                             { .1F, .1F, 1.F }, { .1F, 1.F, .1F },
                             { 1.F, 1.F, .1F }, { .1F, 1.F, 1.F } };

// Attenuation intensity; see https://learnopengl.com/Lighting/Light-casters
struct LightAttenuation {
    uint16_t distance;
    float constant;
    float linear;
    float quadratic;
};
constexpr LightAttenuation lightAttenuation[] = {
    { .distance = 7, .constant = 1.F, .linear = 0.7F, .quadratic = 1.8F },
    { .distance = 13, .constant = 1.F, .linear = 0.35F, .quadratic = 0.44F },
    { .distance = 20, .constant = 1.F, .linear = 0.22F, .quadratic = 0.2F },
    { .distance = 32, .constant = 1.F, .linear = 0.14F, .quadratic = 0.07F },
    { .distance = 50, .constant = 1.F, .linear = 0.09F, .quadratic = 0.032F },
    { .distance = 65, .constant = 1.F, .linear = 0.07F, .quadratic = 0.017F },
    { .distance = 100,
      .constant = 1.F,
      .linear = 0.045F,
      .quadratic = 0.0075F },
    { .distance = 160,
      .constant = 1.F,
      .linear = 0.027F,
      .quadratic = 0.0028F },
    { .distance = 200,
      .constant = 1.F,
      .linear = 0.022F,
      .quadratic = 0.0019F },
    { .distance = 325,
      .constant = 1.F,
      .linear = 0.014F,
      .quadratic = 0.0007F },
    { .distance = 600,
      .constant = 1.F,
      .linear = 0.007F,
      .quadratic = 0.0002F },
    { .distance = 3250,
      .constant = 1.F,
      .linear = 0.0014F,
      .quadratic = 0.000007F }
};

struct LightCube {
    glm::vec3 position;
    glm::vec3 translation;
    uint16_t distance;
    glm::vec3 attenuation;
};
LightCube lightCubes[6];

struct GameObject {
    const char* name;
    const char* path;
    glm::vec3 scale{ 1.F };
    glm::vec3 translation{ 0.F };
};

constexpr GameObject gameObjects[] = {
    { .name = const_cast<char*>("cube"),
      .path = const_cast<char*>("/models/cube/cube.obj"),
      .scale = glm::vec3(.5F),
      .translation = glm::vec3(-3.F, .5003F, 2.25F) },

    { .name = const_cast<char*>("coloredCube"),
      .path = const_cast<char*>("/models/cube/colored_cube.obj"),
      .scale = glm::vec3(.25F),
      .translation = glm::vec3(0.F, 1.003F, -1.F) },

    { .name = const_cast<char*>("bCube"),
      .path = const_cast<char*>("/models/cube/nbcube.obj"),
      .scale = glm::vec3(.25F),
      .translation = glm::vec3(6.25F, 1.003F, 4.25F) },

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
      .translation = glm::vec3(-2.25F, 1.F, 4.5F) },

    { .name = const_cast<char*>("smoothSphere"),
      .path = const_cast<char*>("/models/sphere/smooth_sphere.obj"),
      .scale = glm::vec3(.25F),
      .translation = glm::vec3(2.8F, 1.F, 4.5F) },

    { .name = const_cast<char*>("floor"),
      .path = const_cast<char*>("/models/quad/quad.obj"),
      .scale = glm::vec3({ 10.F, 1.F, 10.F }),
      .translation = glm::vec3(0.F, 0.002F, 0.F) }
};
}  // namespace

namespace game::layer {

using sponge::platform::opengl::renderer::ResourceManager;
using sponge::platform::sdl::core::Application;

MazeLayer::MazeLayer() : Layer("maze") {
    // nothing
}

void MazeLayer::onAttach() {
    for (const auto& gameObject : gameObjects) {
        ResourceManager::loadModel(gameObject.name, gameObject.path);
    }

    camera = game::ResourceManager::createGameCamera(cameraName);
    camera->setPosition(cameraPosition);

    auto shaderName = sponge::platform::opengl::scene::Mesh::getShaderName();
    auto shader = ResourceManager::getShader(shaderName);
    shader->bind();

    shader->setFloat("metallic", metallic ? 1.F : 0.F);
    shader->setFloat("roughness", roughness);
    shader->setFloat("ao", ao);

    shader->setFloat("ambientStrength", ambientStrength);
    shader->setFloat3("lineColor", lineColor);
    shader->setFloat("lineWidth", lineWidth);
    shader->setBoolean("showWireframe", activeWireframe);
    shader->unbind();

    lightCube = std::make_unique<sponge::platform::opengl::scene::LightCube>();

    setNumLights(1);
}

void MazeLayer::onDetach() {
    // nothing
}

bool MazeLayer::onUpdate(const double elapsedTime) {
    UNUSED(elapsedTime);

    auto shaderName = sponge::platform::opengl::scene::Mesh::getShaderName();
    auto shader = ResourceManager::getShader(shaderName);

    shader->bind();

    shader->setInteger("numLights", numLights);

    auto rotateLight =
        rotate(glm::mat4(1.F), static_cast<float>(elapsedTime / 3.F),
               { 0.F, -1.F, 0.F });

    for (int32_t i = 0; i < numLights; ++i) {
        shader->setFloat3("pointLights[" + std::to_string(i) + "].position",
                          lightCubes[i].position);
        shader->setFloat3("pointLights[" + std::to_string(i) + "].attenuation",
                          lightCubes[i].attenuation);

        lightCubes[i].translation =
            glm::vec3(rotateLight * glm::vec4(lightCubes[i].translation, 1.F));
        lightCubes[i].position = glm::vec4(lightCubes[i].translation, 1.F);

        shader->setFloat3("pointLights[" + std::to_string(i) + "].color",
                          lightColors[i]);
    }

    shader->unbind();

    for (const auto& gameObject : gameObjects) {
        shader->bind();

        shader->setFloat3("viewPos", camera->getPosition());
        shader->setMat4("mvp",
                        translate(scale(camera->getMVP(), gameObject.scale),
                                  gameObject.translation));
        shader->setMat4("viewportMatrix", camera->getViewportMatrix());

        ResourceManager::getModel(gameObject.name)->render();

        shader->unbind();
    }

    shaderName = sponge::platform::opengl::scene::LightCube::getShaderName();
    shader = ResourceManager::getShader(shaderName);

    for (int32_t i = 0; i < numLights; ++i) {
        shader->bind();

        shader->setFloat3("lightColor", lightColors[i]);
        shader->setMat4(
            "mvp", scale(translate(camera->getMVP(), lightCubes[i].position),
                         lightCubeScale));

        lightCube->render();

        shader->unbind();
    }

    return true;
}

void MazeLayer::setMetallic(bool metallic) {
    this->metallic = metallic;

    const auto shaderName =
        sponge::platform::opengl::scene::Mesh::getShaderName();
    const auto shader = ResourceManager::getShader(shaderName);
    shader->bind();
    shader->setFloat("metallic", metallic ? 1.F : 0.F);
    shader->unbind();
}

void MazeLayer::setWireframeActive(const bool active) {
    this->activeWireframe = active;

    const auto shaderName =
        sponge::platform::opengl::scene::Mesh::getShaderName();
    const auto shader = ResourceManager::getShader(shaderName);
    shader->bind();
    shader->setBoolean("showWireframe", active);
    shader->unbind();
}

void MazeLayer::setAmbientOcclusion(float ao) {
    this->ao = ao;

    const auto shaderName =
        sponge::platform::opengl::scene::Mesh::getShaderName();
    const auto shader = ResourceManager::getShader(shaderName);
    shader->bind();
    shader->setFloat("ao", ao);
    shader->unbind();
}

void MazeLayer::setAmbientStrength(float strength) {
    this->ambientStrength = strength;

    const auto shaderName =
        sponge::platform::opengl::scene::Mesh::getShaderName();
    const auto shader = ResourceManager::getShader(shaderName);
    shader->bind();
    shader->setFloat("ambientStrength", ambientStrength);
    shader->unbind();
}

void MazeLayer::setRoughness(float roughness) {
    this->roughness = roughness;

    const auto shaderName =
        sponge::platform::opengl::scene::Mesh::getShaderName();
    const auto shader = ResourceManager::getShader(shaderName);
    shader->bind();
    shader->setFloat("roughness", roughness);
    shader->unbind();
}

void MazeLayer::setNumLights(int32_t numLights) {
    this->numLights = numLights;

    for (int32_t i = 0; i < numLights; ++i) {
        lightCubes[i].translation = glm::vec3(
            rotate(glm::mat4(1.F), glm::two_pi<float>() * i / numLights,
                   glm::vec3(0.F, 1.F, 0.F)) *
            glm::vec4(-1.F, 1.5F, -1.F, 1.F));

        int index = 5;
        lightCubes[i].attenuation = glm::vec3(
            lightAttenuation[index].constant, lightAttenuation[index].linear,
            lightAttenuation[index].quadratic);
        lightCubes[i].distance = lightAttenuation[index].distance;
    }
}

void MazeLayer::onEvent(sponge::event::Event& event) {
    sponge::event::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::event::KeyPressedEvent>(
        BIND_EVENT_FN(onKeyPressed));
    dispatcher.dispatch<sponge::event::MouseButtonPressedEvent>(
        BIND_EVENT_FN(onMouseButtonPressed));
    dispatcher.dispatch<sponge::event::MouseButtonReleasedEvent>(
        BIND_EVENT_FN(onMouseButtonReleased));
    dispatcher.dispatch<sponge::event::MouseMovedEvent>(
        BIND_EVENT_FN(onMouseMoved));
    dispatcher.dispatch<sponge::event::MouseScrolledEvent>(
        BIND_EVENT_FN(onMouseScrolled));
    dispatcher.dispatch<sponge::event::WindowResizeEvent>(
        BIND_EVENT_FN(onWindowResize));
}

bool MazeLayer::onKeyPressed(
    const sponge::event::KeyPressedEvent& event) const {
    if (event.getKeyCode() == sponge::input::KeyCode::SpongeKey_W ||
        event.getKeyCode() == sponge::input::KeyCode::SpongeKey_Up) {
        camera->moveForward(event.getElapsedTime() * keyboardSpeed);
    } else if (event.getKeyCode() == sponge::input::KeyCode::SpongeKey_S ||
               event.getKeyCode() == sponge::input::KeyCode::SpongeKey_Down) {
        camera->moveBackward(event.getElapsedTime() * keyboardSpeed);
    } else if (event.getKeyCode() == sponge::input::KeyCode::SpongeKey_A ||
               event.getKeyCode() == sponge::input::KeyCode::SpongeKey_Left) {
        camera->strafeLeft(event.getElapsedTime() * keyboardSpeed);
    } else if (event.getKeyCode() == sponge::input::KeyCode::SpongeKey_D ||
               event.getKeyCode() == sponge::input::KeyCode::SpongeKey_Right) {
        camera->strafeRight(event.getElapsedTime() * keyboardSpeed);
    }
    return false;
}

bool MazeLayer::onMouseButtonPressed(
    const sponge::event::MouseButtonPressedEvent& event) {
    if (event.getMouseButton() == 0) {
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

bool MazeLayer::onMouseMoved(
    const sponge::event::MouseMovedEvent& event) const {
    if (sponge::platform::sdl::input::Mouse::isButtonPressed()) {
        camera->mouseMove({ event.getXRelative() * mouseSpeed,
                            event.getYRelative() * mouseSpeed });
    }
    return true;
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

}  // namespace game::layer
