#include "mazelayer.h"

void MazeLayer::onAttach() {
    Sponge::OpenGLResourceManager::loadShader("assets/shaders/shader.vert", "assets/shaders/shader.frag", "shader");
    Sponge::OpenGLResourceManager::loadModel("assets/models/mountains.obj", "Maze");

    camera = std::make_unique<GameCamera>();
    camera->setPosition(glm::vec3(0.f, 40.f, 70.f));

    auto shader = Sponge::OpenGLResourceManager::getShader("shader");
    shader->bind();

    shader->setFloat3("lightPos", glm::vec3(40.f, 40.f, 40.f));
    shader->setFloat("ambientStrength", 0.3f);
    shader->unbind();
}

void MazeLayer::onDetach() {
    // nothing
}

bool MazeLayer::onUpdate(uint32_t elapsedTime) {
    if (Sponge::Input::isKeyPressed(Sponge::KeyCode::W) || Sponge::Input::isKeyPressed(Sponge::KeyCode::Up)) {
        camera->moveForward(elapsedTime);
    } else if (Sponge::Input::isKeyPressed(Sponge::KeyCode::S) || Sponge::Input::isKeyPressed(Sponge::KeyCode::Down)) {
        camera->moveBackward(elapsedTime);
    } else if (Sponge::Input::isKeyPressed(Sponge::KeyCode::A) || Sponge::Input::isKeyPressed(Sponge::KeyCode::Left)) {
        camera->strafeLeft(elapsedTime);
    } else if (Sponge::Input::isKeyPressed(Sponge::KeyCode::D) || Sponge::Input::isKeyPressed(Sponge::KeyCode::Right)) {
        camera->strafeRight(elapsedTime);
    }

    auto shader = Sponge::OpenGLResourceManager::getShader("shader");
    shader->bind();
    shader->setFloat3("viewPos", camera->getPosition());
    shader->setMat4("mvp", camera->getMVP());
    shader->unbind();

    Sponge::OpenGLResourceManager::getModel("Maze")->render();

    return true;
}

void MazeLayer::onEvent(Sponge::Event& event) {
    Sponge::EventDispatcher dispatcher(event);

    dispatcher.dispatch<Sponge::MouseMovedEvent>(BIND_EVENT_FN(onMouseMoved));
    dispatcher.dispatch<Sponge::MouseScrolledEvent>(BIND_EVENT_FN(onMouseScrolled));
    dispatcher.dispatch<Sponge::WindowResizeEvent>(BIND_EVENT_FN(onWindowResize));
}

bool MazeLayer::onMouseMoved(Sponge::MouseMovedEvent& event) {
    if (Sponge::Input::isButtonPressed()) {
        camera->mouseMove({ event.getX(), event.getY() });
    }
    return true;
}

bool MazeLayer::onMouseScrolled(Sponge::MouseScrolledEvent& event) {
    camera->mouseScroll({ event.getXOffset(), event.getYOffset() });
    return true;
}

bool MazeLayer::onWindowResize(Sponge::WindowResizeEvent& event) {
    camera->setViewportSize(event.getWidth(), event.getHeight());
    return false;
}
