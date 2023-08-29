#include "mazelayer.h"

void MazeLayer::onAttach() {
    auto assetsFolder = sponge::File::getResourceDir();

    sponge::OpenGLResourceManager::loadShader(
        assetsFolder + "/shaders/shader.vert",
        assetsFolder + "/shaders/shader.frag", "shader");
    sponge::OpenGLResourceManager::loadModel(
        assetsFolder + "/models/spider.obj", "Maze");

    camera = std::make_unique<GameCamera>();
    camera->setPosition(glm::vec3(0.F, 40.F, 70.F));

    auto shader = sponge::OpenGLResourceManager::getShader("shader");
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

    auto shader = sponge::OpenGLResourceManager::getShader("shader");
    shader->bind();
    shader->setFloat3("viewPos", camera->getPosition());
    shader->setMat4("mvp", camera->getMVP());
    shader->unbind();

    sponge::OpenGLResourceManager::getModel("Maze")->render();

    return true;
}

void MazeLayer::onEvent(sponge::Event& event) {
    sponge::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::KeyPressedEvent>(BIND_EVENT_FN(onKeyPressed));
    dispatcher.dispatch<sponge::MouseMovedEvent>(BIND_EVENT_FN(onMouseMoved));
    dispatcher.dispatch<sponge::MouseScrolledEvent>(
        BIND_EVENT_FN(onMouseScrolled));
    dispatcher.dispatch<sponge::WindowResizeEvent>(
        BIND_EVENT_FN(onWindowResize));
}

bool MazeLayer::onKeyPressed(const sponge::KeyPressedEvent& event) {
    if (event.getKeyCode() == sponge::KeyCode::SpongeKey_W ||
        event.getKeyCode() == sponge::KeyCode::SpongeKey_Up) {
        camera->moveForward(1);
    } else if (event.getKeyCode() == sponge::KeyCode::SpongeKey_S ||
               event.getKeyCode() == sponge::KeyCode::SpongeKey_Down) {
        camera->moveBackward(1);
    } else if (event.getKeyCode() == sponge::KeyCode::SpongeKey_A ||
               event.getKeyCode() == sponge::KeyCode::SpongeKey_Left) {
        camera->strafeLeft(1);
    } else if (event.getKeyCode() == sponge::KeyCode::SpongeKey_D ||
               event.getKeyCode() == sponge::KeyCode::SpongeKey_Right) {
        camera->strafeRight(1);
    }

    return false;
}

bool MazeLayer::onMouseMoved(const sponge::MouseMovedEvent& event) {
    if (sponge::Input::isButtonPressed()) {
        camera->mouseMove({ event.getXRelative(), event.getYRelative() });
    }
    return true;
}

bool MazeLayer::onMouseScrolled(const sponge::MouseScrolledEvent& event) {
    camera->mouseScroll({ event.getXOffset(), event.getYOffset() });
    return true;
}

bool MazeLayer::onWindowResize(const sponge::WindowResizeEvent& event) {
    camera->setViewportSize(event.getWidth(), event.getHeight());
    return false;
}
