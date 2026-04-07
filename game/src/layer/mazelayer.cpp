#include "layer/mazelayer.hpp"

#include "maze.hpp"
#include "resourcemanager.hpp"
#include "scene/light.hpp"
#include "sponge.hpp"

#include <glm/ext/matrix_transform.hpp>

#include <array>
#include <memory>
#include <string>

namespace {
constexpr auto keyboardSpeed = 0.075F;
constexpr auto mouseSpeed    = 0.125F;

constexpr auto cameraPosition = glm::vec3(0.F, 3.5F, 6.5F);

constexpr auto sunCastsShadow  = true;
constexpr auto sunColor        = glm::vec3(1.F, 1.F, 1.F);
constexpr auto sunDirection    = glm::vec3(0.F, -2.F, 1.333F);
constexpr auto sunEnabled      = true;
constexpr auto sunShadowBias   = 0.004F;
constexpr auto sunShadowMapRes = 2048;

constexpr auto cubeScale = glm::vec3(.1F);

constexpr std::string_view cameraName = "maze";

game::scene::DirectionalLight          directionalLight;
std::array<game::scene::PointLight, 6> pointLights;

struct LightUniforms {
    std::string position;
    std::string color;
    std::string attenuationIndex;
};

std::array<LightUniforms, 6> lightUniformNames;

using game::layer::GameObject;

std::array gameObjects = {
    GameObject{ .name        = "floor",
                .path        = "/models/floor/floor.obj",
                .scale       = glm::vec3(2.F),
                .rotation    = { .angle = 0.F, .axis{ 0.F, 1.F, 0.F } },
                .translation = glm::vec3(0.F, 0.F, 0.F) },

    // GameObject{ .name = "cube1",
    //             .path = "/models/cube/cube-tex.obj",
    //             .scale = glm::vec3(1.F),
    //             .translation = glm::vec3(-1.5F, .85F, -.5F) },
    //
    // GameObject{ .name = "cube2",
    //             .path = "/models/cube/cube-tex.obj",
    //             .scale = glm::vec3(.5F),
    //             .translation = glm::vec3(0.F, 0.F, .5F) },
    //
    // GameObject{ .name = "cube3",
    //             .path = "/models/cube/cube-tex.obj",
    //             .scale = glm::vec3(.25F),
    //             .rotation = { .angle = glm::radians(60.F),
    //                           .axis = glm::vec3(1.F, 0.F, 1.F) },
    //             .translation = glm::vec3(-1.F, 0.25F, 1.F) }

    GameObject{ .name        = "helmet",
                .path        = "/models/helmet/damaged_helmet.obj",
                .scale       = glm::vec3(.5F),
                .rotation    = { .angle = glm::radians(45.F),
                                 .axis  = glm::vec3(0.F, 1.F, 0.F) },
                .translation = glm::vec3(0.F, 0.F, 0.F) }
};
}  // namespace

namespace game::layer {
using sponge::event::Event;
using sponge::event::KeyPressedEvent;
using sponge::event::KeyReleasedEvent;
using sponge::event::MouseButtonPressedEvent;
using sponge::event::MouseButtonReleasedEvent;
using sponge::event::MouseMovedEvent;
using sponge::event::MouseScrolledEvent;
using sponge::event::WindowFocusEvent;
using sponge::event::WindowResizeEvent;
using sponge::input::KeyCode;
using sponge::platform::glfw::core::Application;
using sponge::platform::glfw::core::Input;
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::Cube;
using sponge::platform::opengl::scene::FXAA;
using sponge::platform::opengl::scene::Mesh;
using sponge::platform::opengl::scene::ShadowMap;

MazeLayer::MazeLayer() : Layer("maze") {
    for (int32_t i = 0; i < 6; i++) {
        const std::string base  = "pointLights[" + std::to_string(i) + "].";
        auto&             names = lightUniformNames.at(i);
        names.position          = base + "position";
        names.color             = base + "color";
        names.attenuationIndex  = base + "attenuationIndex";
    }
}

void MazeLayer::onAttach() {
    for (auto& gameObject : gameObjects) {
        // compute the model matrix to avoid recalculating it on every frame
        gameObject.modelMatrix = glm::scale(
            glm::rotate(glm::translate(glm::mat4(1.0f), gameObject.translation),
                        gameObject.rotation.angle, gameObject.rotation.axis),
            gameObject.scale);

        sponge::platform::opengl::scene::ModelCreateInfo modelCreateInfo{
            .name = std::string(gameObject.name),
            .path = std::string(gameObject.path)
        };
        AssetManager::createModel(modelCreateInfo);
    }

    const auto gameCameraCreateInfo =
        scene::GameCameraCreateInfo{ .name = std::string(cameraName) };
    camera = ResourceManager::createGameCamera(gameCameraCreateInfo);
    camera->setViewportSize(Maze::get().getWindow()->getWidth(),
                            Maze::get().getWindow()->getHeight());
    camera->setPosition(cameraPosition);

    const auto shaderName = Mesh::getShaderName();
    const auto shader     = AssetManager::getShader(shaderName);
    shader->bind();

    shader->setFloat("metallic", metallic ? 1.F : 0.F);
    shader->setFloat("roughness", roughness);
    shader->setFloat("ao", ao);

    shader->setFloat("ambientStrength", ambientStrength);

    directionalLight = { .enabled      = sunEnabled,
                         .castShadow   = sunCastsShadow,
                         .color        = sunColor,
                         .direction    = sunDirection,
                         .shadowBias   = sunShadowBias,
                         .shadowMapRes = sunShadowMapRes };

    shader->setBoolean("directionalLight.enabled", directionalLight.enabled);
    shader->setBoolean("directionalLight.castShadow",
                       directionalLight.castShadow);
    shader->setFloat3("directionalLight.direction", directionalLight.direction);
    shader->setFloat3("directionalLight.color", directionalLight.color);
    shader->setFloat("directionalLight.shadowBias",
                     directionalLight.shadowBias);

    shader->unbind();

    shadowMap = std::make_unique<ShadowMap>(directionalLight.shadowMapRes);
    cube      = std::make_unique<Cube>();

    fxaa = std::make_unique<FXAA>(Maze::get().getWindow()->getWidth(),
                                  Maze::get().getWindow()->getHeight());
    fxaa->setEnabled(fxaaEnabled);

    setNumLights(numLights);
    updateShaderLights();
}

void MazeLayer::onDetach() {
    // nothing
}

void MazeLayer::onEvent(Event& event) {
    sponge::event::EventDispatcher dispatcher(event);

    dispatcher.dispatch<KeyPressedEvent>([this](const KeyPressedEvent& ev) {
        return isActive() ? this->onKeyPressed(ev) : false;
    });
    dispatcher.dispatch<KeyReleasedEvent>([this](const KeyReleasedEvent& ev) {
        return isActive() ? this->onKeyReleased(ev) : false;
    });
    dispatcher.dispatch<MouseButtonPressedEvent>(
        [this](const MouseButtonPressedEvent& mbEvent) {
            return isActive() ? this->onMouseButtonPressed(mbEvent) : false;
        });
    dispatcher.dispatch<MouseButtonReleasedEvent>(
        [this](const MouseButtonReleasedEvent& mrEvent) {
            return isActive() ? this->onMouseButtonReleased(mrEvent) : false;
        });
    dispatcher.dispatch<MouseMovedEvent>(
        [this](const MouseMovedEvent& mmEvent) {
            return isActive() ? this->onMouseMoveEvent(mmEvent) : false;
        });
    dispatcher.dispatch<MouseScrolledEvent>(
        [this](const MouseScrolledEvent& msEvent) {
            return isActive() ? this->onMouseScrolled(msEvent) : false;
        });
    dispatcher.dispatch<WindowFocusEvent>(
        [this](const WindowFocusEvent& wfEvent) {
            this->onWindowFocus(wfEvent);
            return false;
        });
    dispatcher.dispatch<WindowResizeEvent>(
        [this](const WindowResizeEvent& wsEvent) {
            return this->onWindowResize(wsEvent);
        });
}

bool MazeLayer::onUpdate(const double elapsedTime) {
    // Update thread only — no GL calls.
    if (Application::get().isEventHandledByImGui()) {
        keyPressed.clear();
        mouseButtonPressed = false;
    }

    updateCamera(elapsedTime);

    // Write the snapshot to the slot not being read by the render thread.
    const uint32_t readSlot  = renderReadIndex.load(std::memory_order_relaxed);
    const uint32_t writeSlot = (readSlot + 1) % 2;
    captureRenderFrame(writeSlot);

    return true;
}

void MazeLayer::captureRenderFrame(const uint32_t slotIndex) {
    auto& frame = renderFrames[slotIndex];

    frame.cameraMVP = camera->getMVP();
    frame.cameraPos = camera->getPosition();

    frame.shadowEnabled    = directionalLight.enabled;
    frame.shadowCastShadow = directionalLight.castShadow;
    frame.lightDirection   = directionalLight.direction;
    if (directionalLight.enabled && directionalLight.castShadow) {
        // Update on update thread to avoid racing render thread
        // bind()/unbind().
        shadowMap->updateLightSpaceMatrix(
            glm::normalize(directionalLight.direction));
        frame.lightSpaceMatrix = shadowMap->getLightSpaceMatrix();
    }

    frame.numLights = numLights;
    for (int32_t i = 0; i < numLights; i++) {
        frame.lightPositions[i]          = pointLights.at(i).position;
        frame.lightColors[i]             = pointLights.at(i).color;
        frame.lightAttenuationIndices[i] = pointLights.at(i).attenuationIndex;
    }

    frame.fxaaEnabled = fxaaEnabled;

    // Static after onAttach(); safe to copy.
    frame.objectModelMatrices.resize(gameObjects.size());
    frame.objectNames.resize(gameObjects.size());
    for (size_t i = 0; i < gameObjects.size(); i++) {
        frame.objectModelMatrices[i] = gameObjects[i].modelMatrix;
        frame.objectNames[i]         = std::string(gameObjects[i].name);
    }

    // Publish slot; release/acquire pair with onRender()'s load.
    renderReadIndex.store(slotIndex, std::memory_order_release);
}

void MazeLayer::onRender() {
    // Render thread only — all GL calls here.
    if (fxaa && pendingResize.load(std::memory_order_acquire)) {
        fxaa->resize(pendingResizeWidth.load(std::memory_order_relaxed),
                     pendingResizeHeight.load(std::memory_order_relaxed));
        pendingResize.store(false, std::memory_order_relaxed);
    }

    // Read the latest snapshot from the update thread.
    const auto& frame =
        renderFrames[renderReadIndex.load(std::memory_order_acquire)];

    if (frame.shadowEnabled && frame.shadowCastShadow) {
        renderSceneToDepthMap(frame);
    }

    if (fxaa && frame.fxaaEnabled) {
        fxaa->begin();
    }

    renderGameObjects(frame);
    renderLightCubes(frame);

    if (fxaa && frame.fxaaEnabled) {
        fxaa->end();
        fxaa->apply();
    }
}

float MazeLayer::getAmbientOcclusion() const {
    return ao;
}

void MazeLayer::setAmbientOcclusion(const float val) {
    ao = val;

    const auto shader = AssetManager::getShader(Mesh::getShaderName());
    shader->bind();
    shader->setFloat("ao", ao);
    shader->unbind();
}

float MazeLayer::getAmbientStrength() const {
    return ambientStrength;
}

void MazeLayer::setAmbientStrength(const float val) {
    ambientStrength = val;

    const auto shader = AssetManager::getShader(Mesh::getShaderName());
    shader->bind();
    shader->setFloat("ambientStrength", ambientStrength);
    shader->unbind();
}

int32_t MazeLayer::getAttenuationIndex() const {
    return attenuationIndex;
}

void MazeLayer::setAttenuationIndex(const int32_t val) {
    attenuationIndex = val;
    setNumLights(numLights);
}

std::shared_ptr<scene::GameCamera> MazeLayer::getCamera() const {
    return camera;
}

bool MazeLayer::getDirectionalLightCastsShadow() const {
    return directionalLight.castShadow;
}

void MazeLayer::setDirectionalLightCastsShadow(const bool value) {
    directionalLight.castShadow = value;

    const auto shader = AssetManager::getShader(Mesh::getShaderName());
    shader->bind();
    shader->setBoolean("directionalLight.castShadow",
                       directionalLight.castShadow);
    shader->unbind();
}

glm::vec3 MazeLayer::getDirectionalLightColor() const {
    return directionalLight.color;
}

void MazeLayer::setDirectionalLightColor(const glm::vec3& color) {
    directionalLight.color = color;

    const auto shader = AssetManager::getShader(Mesh::getShaderName());
    shader->bind();
    shader->setFloat3("directionalLight.color", directionalLight.color);
    shader->unbind();
}

glm::vec3 MazeLayer::getDirectionalLightDirection() const {
    return directionalLight.direction;
}

void MazeLayer::setDirectionalLightDirection(const glm::vec3& direction) {
    directionalLight.direction = direction;

    const auto shader = AssetManager::getShader(Mesh::getShaderName());
    shader->bind();
    shader->setFloat3("directionalLight.direction", directionalLight.direction);
    shader->unbind();
}

bool MazeLayer::getDirectionalLightEnabled() const {
    return directionalLight.enabled;
}

void MazeLayer::setDirectionalLightEnabled(const bool value) {
    directionalLight.enabled = value;

    const auto shader = AssetManager::getShader(Mesh::getShaderName());
    shader->bind();
    shader->setBoolean("directionalLight.enabled", directionalLight.enabled);
    shader->unbind();
}

float MazeLayer::getDirectionalLightShadowBias() const {
    return directionalLight.shadowBias;
}

void MazeLayer::setDirectionalLightShadowBias(const float value) {
    directionalLight.shadowBias = value;

    const auto shader = AssetManager::getShader(Mesh::getShaderName());
    shader->bind();
    shader->setFloat("directionalLight.shadowBias",
                     directionalLight.shadowBias);
    shader->unbind();
}

uint32_t MazeLayer::getDirectionalLightShadowMapRes() const {
    return directionalLight.shadowMapRes;
}

bool MazeLayer::isMetallic() const {
    return metallic;
}

void MazeLayer::setMetallic(const bool val) {
    metallic = val;

    const auto shader = AssetManager::getShader(Mesh::getShaderName());
    shader->bind();
    shader->setFloat("metallic", metallic ? 1.F : 0.F);
    shader->unbind();
}

int32_t MazeLayer::getNumLights() const {
    return numLights;
}

void MazeLayer::setNumLights(const int32_t val) {
    numLights = val;

    for (int32_t i = 0; i < numLights; i++) {
        auto& light    = pointLights.at(i);
        light.color    = glm::vec3(1.F);
        light.position = glm::vec3(rotate(glm::mat4(1.F),
                                          glm::two_pi<float>() * i / numLights,
                                          glm::vec3(0.F, 1.F, 0.F)) *
                                   glm::vec4(0.F, 2.75F, -3.F, 1.F));
        light.attenuationIndex = attenuationIndex;
    }

    updateShaderLights();
}

float MazeLayer::getRoughness() const {
    return roughness;
}

void MazeLayer::setRoughness(const float val) {
    roughness = val;

    const auto shader = AssetManager::getShader(Mesh::getShaderName());
    shader->bind();
    shader->setFloat("roughness", roughness);
    shader->unbind();
}

float MazeLayer::getShadowMapOrthoSize() const {
    return shadowMap->getOrthoSize();
}

void MazeLayer::setShadowMapOrthoSize(float val) const {
    shadowMap->setOrthoSize(val);
}

uint32_t MazeLayer::getShadowMapTextureId() const {
    return shadowMap->getDepthMapTextureId();
}

float MazeLayer::getShadowMapZFar() const {
    return shadowMap->getZFar();
}

void MazeLayer::setShadowMapZFar(float val) const {
    shadowMap->setZFar(val);
}

float MazeLayer::getShadowMapZNear() const {
    return shadowMap->getZNear();
}

void MazeLayer::setShadowMapZNear(float val) const {
    shadowMap->setZNear(val);
}

bool MazeLayer::onKeyPressed(const KeyPressedEvent& event) {
#ifdef ENABLE_IMGUI
    if (event.getKeyCode() == KeyCode::SpongeKey_GraveAccent) {
        const auto imguiLayer = Maze::get().getImGuiLayer();
        imguiLayer->setActive(!imguiLayer->isActive());
        return true;
    }
#endif

    if (event.getKeyCode() == KeyCode::SpongeKey_F) {
        Application::get().toggleFullscreen();
        return true;
    }

    if (event.getKeyCode() == KeyCode::SpongeKey_Escape) {
        Maze::get().getExitLayer()->setActive(true);
#ifdef ENABLE_IMGUI
        Maze::get().getImGuiLayer()->setActive(false);
#endif
        return true;
    }

    keyPressed[event.getKeyCode()] = true;

    return false;
}

bool MazeLayer::onKeyReleased(const KeyReleasedEvent& event) {
    keyPressed.erase(event.getKeyCode());
    return false;
}

void MazeLayer::onWindowFocus(const WindowFocusEvent& event) {
    if (!event.isFocused()) {
        keyPressed.clear();
        mouseButtonPressed = false;
        Application::get().setMouseVisible(true);
    }
}

bool MazeLayer::onMouseButtonPressed(const MouseButtonPressedEvent& event) {
    if (event.getMouseButton() == sponge::input::MouseButton::Button0) {
        Application::get().centerMouse();
        Application::get().setMouseVisible(false);
        mouseButtonPressed = true;
        return true;
    }
    return false;
}

bool MazeLayer::onMouseButtonReleased(const MouseButtonReleasedEvent& event) {
    if (event.getMouseButton() == sponge::input::MouseButton::Button0) {
        Application::get().setMouseVisible(true);
        mouseButtonPressed = false;
        return true;
    }
    return false;
}

bool MazeLayer::onMouseMoveEvent(const MouseMovedEvent& event) {
    if (mouseButtonPressed) {
        const auto xrel = event.getXRelative();
        const auto yrel = event.getYRelative();

        Input::setRelativeCursorPos({ 0.F, 0.F });
        camera->mouseMove({ xrel * mouseSpeed, yrel * mouseSpeed });
        return true;
    }

    return false;
}

bool MazeLayer::onMouseScrolled(const MouseScrolledEvent& event) const {
    camera->mouseScroll({ event.getXOffset(), event.getYOffset() });
    return true;
}

bool MazeLayer::onWindowResize(const WindowResizeEvent& event) const {
    camera->setViewportSize(event.getWidth(), event.getHeight());
    if (fxaa) {
        // Defer GL resize to onRender() on the render thread.
        pendingResizeWidth.store(event.getWidth(), std::memory_order_relaxed);
        pendingResizeHeight.store(event.getHeight(), std::memory_order_relaxed);
        pendingResize.store(true, std::memory_order_release);
    }
    return false;
}

void MazeLayer::renderGameObjects(const thread::MazeRenderFrame& frame) const {
    glViewport(0, 0, Maze::get().getWindow()->getWidth(),
               Maze::get().getWindow()->getHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto shader = AssetManager::getShader(Mesh::getShaderName());

    for (size_t i = 0; i < frame.objectNames.size(); i++) {
        const auto& modelMatrix = frame.objectModelMatrices[i];
        const auto& name        = frame.objectNames[i];

        shader->bind();
        shader->setFloat3("viewPos", frame.cameraPos);
        shader->setMat4("mvp", frame.cameraMVP * modelMatrix);
        shader->setMat4("model", modelMatrix);

        if (frame.shadowEnabled && frame.shadowCastShadow) {
            shader->setMat4("lightSpaceMatrix", frame.lightSpaceMatrix);
            shader->setInteger("shadowMap", 1);
            shadowMap->activateAndBindDepthMap(1);
        }

        AssetManager::getModel(name)->render(shader);
        shader->unbind();
    }
}

void MazeLayer::renderLightCubes(const thread::MazeRenderFrame& frame) const {
    const auto shader = AssetManager::getShader(Cube::getShaderName());

    for (int32_t i = 0; i < frame.numLights; i++) {
        shader->bind();
        shader->setFloat3("lightColor", frame.lightColors[i]);
        shader->setMat4(
            "mvp", scale(translate(frame.cameraMVP, frame.lightPositions[i]),
                         cubeScale));
        cube->render();
        shader->unbind();
    }
}

void MazeLayer::renderSceneToDepthMap(
    const thread::MazeRenderFrame& frame) const {
    shadowMap->bind();

    // Use snapshot data only — render thread must not write to shadowMap.
    const auto shader = AssetManager::getShader(ShadowMap::getShaderName());

    for (size_t i = 0; i < frame.objectNames.size(); i++) {
        shader->bind();
        shader->setMat4("lightSpaceMatrix", frame.lightSpaceMatrix);
        shader->setMat4("model", frame.objectModelMatrices[i]);

        AssetManager::getModel(frame.objectNames[i])->render(shader);
        shader->unbind();
    }

    shadowMap->unbind();
}

void MazeLayer::updateCamera(const double elapsedTime) const {
    if (isKeyPressed(KeyCode::SpongeKey_W) ||
        isKeyPressed(KeyCode::SpongeKey_Up)) {
        camera->moveForward(elapsedTime * keyboardSpeed);
    }

    if (isKeyPressed(KeyCode::SpongeKey_S) ||
        isKeyPressed(KeyCode::SpongeKey_Down)) {
        camera->moveBackward(elapsedTime * keyboardSpeed);
    }

    if (isKeyPressed(KeyCode::SpongeKey_A) ||
        isKeyPressed(KeyCode::SpongeKey_Left)) {
        camera->strafeLeft(elapsedTime * keyboardSpeed);
    }

    if (isKeyPressed(KeyCode::SpongeKey_D) ||
        isKeyPressed(KeyCode::SpongeKey_Right)) {
        camera->strafeRight(elapsedTime * keyboardSpeed);
    }
}

void MazeLayer::updateShaderLights() const {
    const auto shader = AssetManager::getShader(Mesh::getShaderName());

    shader->bind();

    shader->setInteger("numLights", numLights);

    for (int32_t i = 0; i < numLights; i++) {
        shader->setFloat3(lightUniformNames.at(i).position,
                          pointLights.at(i).position);
        shader->setFloat3(lightUniformNames.at(i).color,
                          pointLights.at(i).color);
        shader->setInteger(lightUniformNames.at(i).attenuationIndex,
                           pointLights.at(i).attenuationIndex);
    }

    shader->unbind();
}

bool MazeLayer::isKeyPressed(const KeyCode key) const {
    const auto it = keyPressed.find(key);
    return it != keyPressed.end() && it->second;
}

bool MazeLayer::isFxaaEnabled() const {
    return fxaaEnabled;
}

void MazeLayer::setFxaaEnabled(const bool val) {
    fxaaEnabled = val;
    if (fxaa) {
        fxaa->setEnabled(val);
    }
}
}  // namespace game::layer
