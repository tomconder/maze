#include "mazelayer.h"

constexpr std::string_view modelName = "maze";
constexpr std::string_view mazeShader = "mesh";

MazeLayer::MazeLayer() : Layer("maze") {
    // nothing
}

void MazeLayer::onAttach() {
    const auto assetsFolder = sponge::File::getResourceDir();

    sponge::renderer::OpenGLResourceManager::loadShader(
        assetsFolder + "/shaders/shader.vert",
        assetsFolder + "/shaders/shader.frag",
        assetsFolder + "/shaders/shader.geom", mazeShader.data());
    sponge::renderer::OpenGLResourceManager::loadModel(
        assetsFolder + "/models/mountains.obj", modelName.data());

    camera = std::make_unique<GameCamera>();
    camera->setPosition(glm::vec3(0.F, 40.F, 70.F));

    const auto shader =
        sponge::renderer::OpenGLResourceManager::getShader(mazeShader.data());
    shader->bind();

    shader->setFloat3("lightPos", glm::vec3(40.F, 40.F, 40.F));
    shader->setFloat("ambientStrength", .3F);
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
        sponge::renderer::OpenGLResourceManager::getShader(mazeShader.data());
    shader->bind();
    shader->setFloat3("viewPos", camera->getPosition());
    shader->setMat4("mvp", camera->getMVP());
    shader->setMat4("viewportMatrix", camera->getViewportMatrix());
    shader->unbind();

    sponge::renderer::OpenGLResourceManager::getModel(modelName.data())
        ->render();

    return true;
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

bool MazeLayer::onMouseButtonPressed(
    const sponge::event::MouseButtonPressedEvent& event) {
    if (event.getMouseButton() == 0) {
        sponge::SDLEngine::get().setMouseVisible(false);
        return true;
    }
    return false;
}

bool MazeLayer::onMouseButtonReleased(
    const sponge::event::MouseButtonReleasedEvent& event) {
    if (event.getMouseButton() == 0) {
        sponge::SDLEngine::get().setMouseVisible(true);
        return true;
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
