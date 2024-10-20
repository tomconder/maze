#include "mazelayer.hpp"
#include "resourcemanager.hpp"
#include <glm/ext/matrix_transform.hpp>

namespace {
constexpr auto ambientStrength = .3F;
constexpr auto keyboardSpeed = .1F;
constexpr auto mouseSpeed = .1F;

constexpr auto lightCubeScale = glm::vec3(.2F);
constexpr auto lightPos = glm::vec3(1.F, 5.F, 2.F);
// constexpr auto lightColor = glm::vec3(.8392F, .2823F, .8413F);
constexpr auto lightColor = glm::vec3(1.F, 1.F, 1.F);
constexpr auto modelScale = glm::vec3(1.F);
constexpr auto modelTranslation = glm::vec3(0.F, .5003F, 0.F);

constexpr char cameraName[] = "maze";
constexpr char modelName[] = "model";
constexpr char modelPath[] = "/models/cube/cube.obj";
}  // namespace

namespace game::layer {

using sponge::platform::opengl::renderer::ResourceManager;
using sponge::platform::sdl::core::Application;

MazeLayer::MazeLayer() : Layer("maze") {
    // nothing
}

void MazeLayer::onAttach() {
    ResourceManager::loadModel(modelName, modelPath);

    camera = game::ResourceManager::createGameCamera(cameraName);
    camera->setPosition(glm::vec3(0.F, 11.F, 14.F));

    auto shaderName = sponge::platform::opengl::scene::Mesh::getShaderName();
    auto shader = ResourceManager::getShader(shaderName);
    shader->bind();

    shader->setFloat3("lightPos", lightPos);
    shader->setFloat3("lightColor", lightColor);
    shader->setFloat("ambientStrength", ambientStrength);
    shader->setBoolean("showWireframe", activeWireframe);
    shader->setFloat3("lineColor", glm::vec3(0.05F, .75F, 0.F));
    shader->setFloat("lineWidth", .3F);
    shader->unbind();

    lightCube = std::make_unique<sponge::platform::opengl::scene::LightCube>();
    shaderName = sponge::platform::opengl::scene::LightCube::getShaderName();
    shader = ResourceManager::getShader(shaderName);
    shader->bind();
    shader->setFloat3("lightColor", lightColor);
    shader->unbind();
}

void MazeLayer::onDetach() {
    // nothing
}

bool MazeLayer::onUpdate(const double elapsedTime) {
    UNUSED(elapsedTime);

    auto shaderName = sponge::platform::opengl::scene::Mesh::getShaderName();
    auto shader = ResourceManager::getShader(shaderName);
    shader->bind();
    shader->setFloat3("viewPos", camera->getPosition());
    shader->setMat4("mvp", translate(scale(camera->getMVP(), modelScale),
                                     modelTranslation));
    shader->setMat4("viewportMatrix", camera->getViewportMatrix());
    shader->unbind();

    shaderName = sponge::platform::opengl::scene::LightCube::getShaderName();
    shader = ResourceManager::getShader(shaderName);
    shader->bind();
    shader->setMat4(
        "mvp", scale(translate(camera->getMVP(), lightPos), lightCubeScale));
    shader->unbind();

    ResourceManager::getModel(modelName)->render();

    lightCube->render();

    return true;
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
