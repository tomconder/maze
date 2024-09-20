#include "mazelayer.hpp"
#include "resourcemanager.hpp"
#include <glm/ext/matrix_transform.hpp>

const std::string modelName{ "maze" };
const std::string mazeShader{ "mesh" };
const std::string cameraName{ "maze" };

namespace game::layer {

MazeLayer::MazeLayer() : Layer("maze") {
    // nothing
}

void MazeLayer::onAttach() {
    sponge::platform::opengl::renderer::ResourceManager::loadShader(
        "/shaders/shader.vert", "/shaders/shader.frag", "/shaders/shader.geom",
        mazeShader);
    sponge::platform::opengl::renderer::ResourceManager::loadModel(
        mazeShader, "/models/cube/cube.obj", modelName);

    camera = ResourceManager::createGameCamera(cameraName);
    camera->setPosition(glm::vec3(0.F, 11.F, 14.F));

    const auto shader =
        sponge::platform::opengl::renderer::ResourceManager::getShader(
            mazeShader);
    shader->bind();

    shader->setFloat3("lightPos", glm::vec3(14.F, 4.F, 14.F));
    shader->setFloat("ambientStrength", .3F);
    shader->setBoolean("showWireframe", activeWireframe);
    shader->setFloat3("lineColor", glm::vec3(0.05F, .75F, 0.F));
    shader->setFloat("lineWidth", .3F);
    shader->unbind();
}

void MazeLayer::onDetach() {
    // nothing
}

bool MazeLayer::onUpdate(const double elapsedTime) {
    UNUSED(elapsedTime);

    const auto shader =
        sponge::platform::opengl::renderer::ResourceManager::getShader(
            mazeShader);
    shader->bind();
    shader->setFloat3("viewPos", camera->getPosition());
    shader->setMat4(
        "mvp",
        camera->getMVP() * translate(glm::mat4(1.F), glm::vec3(0, .50003F, 0)));
    shader->setMat4("viewportMatrix", camera->getViewportMatrix());
    shader->unbind();

    sponge::platform::opengl::renderer::ResourceManager::getModel(modelName)
        ->render();

    return true;
}

void MazeLayer::setWireframeActive(const bool active) {
    this->activeWireframe = active;

    const auto shader =
        sponge::platform::opengl::renderer::ResourceManager::getShader(
            mazeShader);
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
        sponge::platform::sdl::Application::get().setMouseVisible(false);
        return true;
    }
    return false;
}

bool MazeLayer::onMouseButtonReleased(
    const sponge::event::MouseButtonReleasedEvent& event) {
    if (event.getMouseButton() == 0) {
        sponge::platform::sdl::Application::get().setMouseVisible(true);
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
