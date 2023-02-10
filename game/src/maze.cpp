#include "maze.h"

#ifdef EMSCRIPTEN
#include <new>
#endif

#include <glm/ext/matrix_clip_space.hpp>

#include "core/log.h"
#include "platform/sdl/sdlengine.h"
#include "renderer/opengl/openglresourcemanager.h"
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
    OpenGLResourceManager::loadShader("assets/shaders/shader.vert", "assets/shaders/shader.frag", "shader");
    OpenGLResourceManager::loadShader("assets/shaders/sprite.vert", "assets/shaders/sprite.frag", "sprite");
    OpenGLResourceManager::loadShader("assets/shaders/text.vert", "assets/shaders/text.frag", "text");

    OpenGLResourceManager::loadModel("assets/models/mountains.obj", "Maze");

    OpenGLResourceManager::loadTexture("assets/images/coffee.png", "coffee");

    OpenGLResourceManager::loadFont("assets/fonts/league-gothic/league-gothic.fnt", "league-gothic", w, h);

    std::shared_ptr<OpenGLShader> shader = OpenGLResourceManager::getShader("shader");
    shader->bind();

    SPONGE_INFO("Setting camera for {}x{}", w, h);
    adjustAspectRatio(w, h);
    renderer->setViewport(offsetx, offsety, w, h);

    camera = std::make_unique<GameCamera>(80.0f, static_cast<float>(w), static_cast<float>(h), 0.01f, 18000.0f);

    camera->setPosition(glm::vec3(0.f, 40.f, 70.f));

    glm::mat4 view = camera->getViewMatrix();
    shader->setMat4("view", view);

    glm::mat4 projection = camera->getProjection();
    shader->setMat4("projection", projection);

    shader->setFloat3("lightPos", glm::vec3(40.f, 40.f, 40.f));
    shader->setFloat("ambientStrength", 0.3f);

    sprite = std::make_unique<OpenGLSprite>(w, h);

    return true;
}

bool Maze::onUserUpdate(Uint32 elapsedTime) {
    if (input.wasKeyPressed(SDL_SCANCODE_ESCAPE) || input.wasKeyPressed(SDL_SCANCODE_Q)) {
        return false;
    }

    if (KEY_PRESSED_OR_HOLD(SDL_SCANCODE_W) || KEY_PRESSED_OR_HOLD(SDL_SCANCODE_UP)) {
        camera->moveForward(elapsedTime);
    } else if (KEY_PRESSED_OR_HOLD(SDL_SCANCODE_S) || KEY_PRESSED_OR_HOLD(SDL_SCANCODE_DOWN)) {
        camera->moveBackward(elapsedTime);
    } else if (KEY_PRESSED_OR_HOLD(SDL_SCANCODE_A) || KEY_PRESSED_OR_HOLD(SDL_SCANCODE_LEFT)) {
        camera->strafeLeft(elapsedTime);
    } else if (KEY_PRESSED_OR_HOLD(SDL_SCANCODE_D) || KEY_PRESSED_OR_HOLD(SDL_SCANCODE_RIGHT)) {
        camera->strafeRight(elapsedTime);
    }

    if (input.wasKeyPressed(SDL_SCANCODE_F)) {
        graphics->toggleFullscreen();
    }

    if (input.isButtonPressed()) {
        camera->mouseMove(input.getMoveDelta());
    }

    camera->mouseScroll(input.getScrollDelta());

    std::shared_ptr<OpenGLShader> shader = OpenGLResourceManager::getShader("shader");
    shader->bind();
    shader->setMat4("view", camera->getViewMatrix());
    shader->setMat4("projection", camera->getProjection());
    shader->setFloat3("viewPos", camera->getPosition());

    auto model = glm::mat4(1.f);
    shader->setMat4("model", model);

    OpenGLResourceManager::getModel("Maze")->render();

    sprite->render("coffee", glm::vec2(static_cast<float>(w) - 68.f, static_cast<float>(h) - 68.f),
                   glm::vec2(64.f, 64.f));

    OpenGLResourceManager::getFont("league-gothic")
        ->renderText("Press [Q] to exit", 12.f, static_cast<float>(h) - 12.f, 28, glm::vec3(0.5, 0.9f, 1.0f));

    return true;
}

bool Maze::onUserResize(int width, int height) {
    camera->setViewportSize(width, height);

    glm::mat projection = camera->getProjection();
    auto shader = OpenGLResourceManager::getShader("shader");
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    projection = glm::ortho(0.f, static_cast<float>(width), static_cast<float>(height), 0.f);
    shader = OpenGLResourceManager::getShader("sprite");
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    projection = glm::ortho(0.f, static_cast<float>(width), 0.f, static_cast<float>(height));
    shader = OpenGLResourceManager::getShader("text");
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    return true;
}

bool Maze::onUserDestroy() {
    return true;
}
