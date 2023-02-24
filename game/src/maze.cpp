#include "maze.h"

#ifdef EMSCRIPTEN
#include <new>
#endif

#include "sponge.h"
#include "version.h"

#define KEY_PRESSED_OR_HOLD(key) input.wasKeyPressed(key) || input.isKeyHeld(key)

Maze::Maze(int screenWidth, int screenHeight) {
    std::string base = "Maze ";
    appName = base + MAZE_VERSION;
    SPONGE_INFO("Maze {}", MAZE_VERSION);
    w = screenWidth;
    h = screenHeight;
}

bool Maze::onUserCreate() {
    Sponge::OpenGLResourceManager::loadShader("assets/shaders/shader.vert", "assets/shaders/shader.frag", "shader");
    Sponge::OpenGLResourceManager::loadShader("assets/shaders/sprite.vert", "assets/shaders/sprite.frag", "sprite");
    Sponge::OpenGLResourceManager::loadShader("assets/shaders/text.vert", "assets/shaders/text.frag", "text");

    Sponge::OpenGLResourceManager::loadModel("assets/models/mountains.obj", "Maze");

    Sponge::OpenGLResourceManager::loadTexture("assets/images/coffee.png", "coffee");

    Sponge::OpenGLResourceManager::loadFont("assets/fonts/league-gothic/league-gothic.fnt", "league-gothic");

    auto shader = Sponge::OpenGLResourceManager::getShader("shader");
    shader->bind();

    SPONGE_INFO("Setting camera for {}x{}", w, h);
    adjustAspectRatio(w, h);
    renderer->setViewport(offsetx, offsety, w, h);

    camera = std::make_unique<GameCamera>(80.0f, static_cast<float>(w), static_cast<float>(h), 1.f, 18000.0f);
    camera->setPosition(glm::vec3(0.f, 40.f, 70.f));

    shader->setFloat3("lightPos", glm::vec3(40.f, 40.f, 40.f));
    shader->setFloat("ambientStrength", 0.3f);
    shader->unbind();

    logo = std::make_unique<Sponge::OpenGLSprite>("coffee");

    orthoCamera = std::make_unique<Sponge::OrthoCamera>(static_cast<float>(w), static_cast<float>(h));

    shader = Sponge::OpenGLResourceManager::getShader("sprite");
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    shader = Sponge::OpenGLResourceManager::getShader("text");
    shader->bind();
    shader->setMat4("projection", orthoCamera->getProjection());
    shader->unbind();

    return true;
}

bool Maze::onUserUpdate(Uint32 elapsedTime) {
    if (input.wasKeyPressed(Sponge::KeyCode::FEscape) || input.wasKeyPressed(Sponge::KeyCode::Q)) {
        return false;
    }

    if (KEY_PRESSED_OR_HOLD(Sponge::KeyCode::W) || KEY_PRESSED_OR_HOLD(Sponge::KeyCode::Up)) {
        camera->moveForward(elapsedTime);
    } else if (KEY_PRESSED_OR_HOLD(Sponge::KeyCode::S) || KEY_PRESSED_OR_HOLD(Sponge::KeyCode::Down)) {
        camera->moveBackward(elapsedTime);
    } else if (KEY_PRESSED_OR_HOLD(Sponge::KeyCode::A) || KEY_PRESSED_OR_HOLD(Sponge::KeyCode::Left)) {
        camera->strafeLeft(elapsedTime);
    } else if (KEY_PRESSED_OR_HOLD(Sponge::KeyCode::D) || KEY_PRESSED_OR_HOLD(Sponge::KeyCode::Right)) {
        camera->strafeRight(elapsedTime);
    }

    if (input.wasKeyPressed(Sponge::KeyCode::F)) {
        graphics->toggleFullscreen();
    }

    if (input.isButtonPressed()) {
        camera->mouseMove(input.getMoveDelta());
    }

    if (input.wasMouseScrolled()) {
        camera->mouseScroll(input.getScrollDelta());
    }

    auto shader = Sponge::OpenGLResourceManager::getShader("shader");
    shader->bind();
    shader->setFloat3("viewPos", camera->getPosition());
    shader->setMat4("mvp", camera->getMVP());
    shader->unbind();

    Sponge::OpenGLResourceManager::getModel("Maze")->render();

    logo->render({ static_cast<float>(w) - 76.f, 12.f }, { 64.f, 64.f });

    Sponge::OpenGLResourceManager::getFont("league-gothic")
        ->render("Press [Q] to exit", { 12.f, static_cast<float>(h) - 12.f }, 28, { 0.5, 0.9f, 1.0f });

    return true;
}

bool Maze::onUserResize(int width, int height) {
    camera->setViewportSize(width, height);

    orthoCamera->setWidthAndHeight(width, height);

    auto projection = orthoCamera->getProjection();
    auto shader = Sponge::OpenGLResourceManager::getShader("sprite");
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    shader = Sponge::OpenGLResourceManager::getShader("text");
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    return true;
}

bool Maze::onUserDestroy() {
    return true;
}
