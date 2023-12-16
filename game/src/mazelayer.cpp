#include "mazelayer.h"

constexpr std::string_view modelName = "maze";
constexpr std::string_view mazeShader = "shader";

MazeLayer::MazeLayer() : Layer("maze") {
    // nothing
}

void MazeLayer::onAttach() {
    auto assetsFolder = sponge::File::getResourceDir();

    sponge::graphics::renderer::OpenGLResourceManager::loadShader(
        assetsFolder + "/shaders/shader.vert",
        assetsFolder + "/shaders/shader.frag", mazeShader.data());
    sponge::graphics::renderer::OpenGLResourceManager::loadModel(
        assetsFolder + "/models/mountains.obj", modelName.data());

    camera = std::make_unique<GameCamera>();
    camera->setPosition(glm::vec3(0.F, 40.F, 70.F));

    auto shader = sponge::graphics::renderer::OpenGLResourceManager::getShader(
        mazeShader.data());
    shader->bind();

    shader->setFloat3("lightPos", glm::vec3(40.F, 40.F, 40.F));
    shader->setFloat("ambientStrength", 0.3F);
    shader->unbind();
}

void MazeLayer::onDetach() {
    // nothing
}

bool MazeLayer::onUpdate(uint32_t elapsedTime) {
    UNUSED(elapsedTime);

    auto shader = sponge::graphics::renderer::OpenGLResourceManager::getShader(
        mazeShader.data());
    shader->bind();
    shader->setFloat3("viewPos", camera->getPosition());
    shader->setMat4("mvp", camera->getMVP());
    shader->unbind();

    sponge::graphics::renderer::OpenGLResourceManager::getModel(
        modelName.data())
        ->render();

    return true;
}

void MazeLayer::onEvent(sponge::event::Event& event) {
    sponge::event::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::event::KeyPressedEvent>(
        BIND_EVENT_FN(onKeyPressed));
    dispatcher.dispatch<sponge::event::MouseMovedEvent>(
        BIND_EVENT_FN(onMouseMoved));
    dispatcher.dispatch<sponge::event::MouseScrolledEvent>(
        BIND_EVENT_FN(onMouseScrolled));
    dispatcher.dispatch<sponge::event::WindowResizeEvent>(
        BIND_EVENT_FN(onWindowResize));
}

bool MazeLayer::onKeyPressed(const sponge::event::KeyPressedEvent& event) {
    if (event.getKeyCode() == sponge::KeyCode::SpongeKey_W ||
        event.getKeyCode() == sponge::KeyCode::SpongeKey_Up) {
        camera->moveForward(event.getElapsedTime() * keyboardSpeed);
    } else if (event.getKeyCode() == sponge::KeyCode::SpongeKey_S ||
               event.getKeyCode() == sponge::KeyCode::SpongeKey_Down) {
        camera->moveBackward(event.getElapsedTime() * keyboardSpeed);
    } else if (event.getKeyCode() == sponge::KeyCode::SpongeKey_A ||
               event.getKeyCode() == sponge::KeyCode::SpongeKey_Left) {
        camera->strafeLeft(event.getElapsedTime() * keyboardSpeed);
    } else if (event.getKeyCode() == sponge::KeyCode::SpongeKey_D ||
               event.getKeyCode() == sponge::KeyCode::SpongeKey_Right) {
        camera->strafeRight(event.getElapsedTime() * keyboardSpeed);
    }
    return false;
}

bool MazeLayer::onMouseMoved(const sponge::event::MouseMovedEvent& event) {
    if (sponge::Input::isButtonPressed()) {
        camera->mouseMove({ event.getXRelative() * mouseSpeed,
                            event.getYRelative() * mouseSpeed });
    }
    return true;
}

bool MazeLayer::onMouseScrolled(
    const sponge::event::MouseScrolledEvent& event) {
    camera->mouseScroll({ event.getXOffset(), event.getYOffset() });
    return true;
}

bool MazeLayer::onWindowResize(const sponge::event::WindowResizeEvent& event) {
    camera->setViewportSize(event.getWidth(), event.getHeight());
    return false;
}
