#include "mazelayer.h"

#if __APPLE__
#include <CoreFoundation/CFBundle.h>

std::string getResourcesDir() {
    CFURLRef resourceURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
    char resourcePath[PATH_MAX];
    if (CFURLGetFileSystemRepresentation(resourceURL, true,
                                         (UInt8*)resourcePath, PATH_MAX)) {
        if (resourceURL != NULL) {
            CFRelease(resourceURL);
        }
        return resourcePath;
    }

    return "../Resources";
}
#endif

void MazeLayer::onAttach() {
#if __APPLE__
    std::string assetsFolder = getResourcesDir();
#else
    std::string assetsFolder = "assets";
#endif

    sponge::OpenGLResourceManager::loadShader(
        assetsFolder + "/shaders/shader.vert", assetsFolder + "/shaders/shader.frag", "shader");
    sponge::OpenGLResourceManager::loadModel(assetsFolder + "/models/mountains.obj",
                                             "Maze");

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
    if (sponge::Input::isKeyPressed(sponge::KeyCode::SpongeKey_W) ||
        sponge::Input::isKeyPressed(sponge::KeyCode::SpongeKey_Up)) {
        camera->moveForward(elapsedTime);
    } else if (sponge::Input::isKeyPressed(sponge::KeyCode::SpongeKey_S) ||
               sponge::Input::isKeyPressed(sponge::KeyCode::SpongeKey_Down)) {
        camera->moveBackward(elapsedTime);
    } else if (sponge::Input::isKeyPressed(sponge::KeyCode::SpongeKey_A) ||
               sponge::Input::isKeyPressed(sponge::KeyCode::SpongeKey_Left)) {
        camera->strafeLeft(elapsedTime);
    } else if (sponge::Input::isKeyPressed(sponge::KeyCode::SpongeKey_D) ||
               sponge::Input::isKeyPressed(sponge::KeyCode::SpongeKey_Right)) {
        camera->strafeRight(elapsedTime);
    }

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

    dispatcher.dispatch<sponge::MouseMovedEvent>(BIND_EVENT_FN(onMouseMoved));
    dispatcher.dispatch<sponge::MouseScrolledEvent>(
        BIND_EVENT_FN(onMouseScrolled));
    dispatcher.dispatch<sponge::WindowResizeEvent>(
        BIND_EVENT_FN(onWindowResize));
}

bool MazeLayer::onMouseMoved(const sponge::MouseMovedEvent& event) {
    if (sponge::Input::isButtonPressed()) {
        camera->mouseMove({ event.getX(), event.getY() });
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
