#include "layer/mazelayer.hpp"

#include "core/settings.hpp"
#include "maze.hpp"
#include "resourcemanager.hpp"
#include "scene/light.hpp"
#include "sponge.hpp"

#include <glm/ext/matrix_transform.hpp>

#include <array>
#include <memory>
#include <random>
#include <string>

namespace {
constexpr auto cameraPosition = glm::vec3(0.F, 3.5F, 6.5F);

constexpr auto dirLightCastsShadow = true;
constexpr auto dirLightColor       = glm::vec3(1.F, 1.F, 1.F);
constexpr auto dirLightDirection   = glm::vec3(0.F, -2.F, 1.333F);
constexpr auto dirLightEnabled     = true;
constexpr auto defaultShadowMapRes = 1024U;

constexpr auto cubeScale = glm::vec3(.1F);

constexpr std::string_view cameraName = "maze";

constexpr int32_t maxPointLights = 500;

game::scene::DirectionalLight                       directionalLight;
std::array<game::scene::PointLight, maxPointLights> pointLights;

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
    //             .rotation    = { .angle = 0.F, .axis{ 0.F, 1.F, 0.F } },
    //             .translation = glm::vec3(-1.5F, .85F, -.5F) },
    //
    // GameObject{ .name = "cube2",
    //             .path = "/models/cube/cube-tex.obj",
    //             .scale = glm::vec3(.5F),
    //             .rotation    = { .angle = 0.F, .axis{ 0.F, 1.F, 0.F } },
    //             .translation = glm::vec3(0.F, 0.F, .5F) },

    GameObject{ .name        = "cube3",
                .path        = "/models/cube/cube-tex.obj",
                .scale       = glm::vec3(.25F),
                .rotation    = { .angle = glm::radians(60.F),
                                 .axis  = glm::vec3(1.F, 0.F, 1.F) },
                .translation = glm::vec3(-1.F, 0.25F, 1.F),
                .emissive    = glm::vec3(1.5F, 1.2F, 0.5F) },

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
using sponge::event::MouseButtonPressedEvent;
using sponge::event::MouseButtonReleasedEvent;
using sponge::event::MouseScrolledEvent;
using sponge::event::WindowFocusEvent;
using sponge::event::WindowResizeEvent;
using sponge::input::GameAction;
using sponge::input::InputSnapshot;
using sponge::platform::glfw::core::Application;
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::Bloom;
using sponge::platform::opengl::scene::ClusteredLights;
using sponge::platform::opengl::scene::Cube;
using sponge::platform::opengl::scene::FXAA;
using sponge::platform::opengl::scene::Mesh;
using sponge::platform::opengl::scene::ShadowMap;

MazeLayer::MazeLayer() : Layer("maze") {}

void MazeLayer::onAttach() {
    for (auto& gameObject : gameObjects) {
        // compute the model matrix once; it never changes after onAttach
        objectModelMatrices.push_back(glm::scale(
            glm::rotate(glm::translate(glm::mat4(1.0f), gameObject.translation),
                        gameObject.rotation.angle, gameObject.rotation.axis),
            gameObject.scale));
        objectEmissives.push_back(gameObject.emissive);

        sponge::platform::opengl::scene::ModelCreateInfo modelCreateInfo{
            .name = std::string(gameObject.name),
            .path = std::string(gameObject.path)
        };
        objectModels.push_back(AssetManager::createModel(modelCreateInfo));
    }

    const auto gameCameraCreateInfo =
        scene::GameCameraCreateInfo{ .name = std::string(cameraName) };
    camera = ResourceManager::createGameCamera(gameCameraCreateInfo);
    camera->setViewportSize(Maze::get().getWindow()->getWidth(),
                            Maze::get().getWindow()->getHeight());
    camera->setPosition(cameraPosition);

    const auto shader = Mesh::getShader();
    shader->bind();

    shader->setFloat("metallic", metallic ? 1.F : 0.F);
    shader->setFloat("roughness", roughness);
    shader->setFloat("ao", ao);

    shader->setFloat("ambientStrength", ambientStrength);

    const auto savedShadowRes = sponge::core::Settings::getUInt32(
        "video.shadowRes", defaultShadowMapRes);

    directionalLight = { .enabled      = dirLightEnabled,
                         .castShadow   = dirLightCastsShadow,
                         .color        = dirLightColor,
                         .direction    = dirLightDirection,
                         .shadowMapRes = savedShadowRes };

    shader->setBoolean("directionalLight.enabled", directionalLight.enabled);
    shader->setBoolean("directionalLight.castShadow",
                       directionalLight.castShadow);
    shader->setFloat3("directionalLight.direction", directionalLight.direction);
    shader->setFloat3("directionalLight.color", directionalLight.color);
    shader->setFloat("evsmBleedThreshold", 0.2F);

    shader->unbind();

    shadowMap = std::make_unique<ShadowMap>(directionalLight.shadowMapRes);
    cube      = std::make_unique<Cube>();

    fxaa = std::make_unique<FXAA>(Maze::get().getWindow()->getWidth(),
                                  Maze::get().getWindow()->getHeight());
    fxaa->setEnabled(fxaaEnabled);

    bloom = std::make_unique<Bloom>(Maze::get().getWindow()->getWidth(),
                                    Maze::get().getWindow()->getHeight());

    queueResize(Maze::get().getWindow()->getWidth(),
                Maze::get().getWindow()->getHeight());

    const auto w = static_cast<int>(Maze::get().getWindow()->getWidth());
    const auto h = static_cast<int>(Maze::get().getWindow()->getHeight());
    screenWidth  = w;
    screenHeight = h;
    clusteredLights =
        std::make_unique<ClusteredLights>(camera->getNear(), camera->getFar());

    depthPrepassShader = AssetManager::createShader({
        .name               = "depthprepass",
        .vertexShaderPath   = "/shaders/glsl/depthprepass.vert.glsl",
        .fragmentShaderPath = "/shaders/glsl/depthprepass.frag.glsl",
        .geometryShaderPath = "",
    });
    createDepthPrepassFbo(w, h);

    shader->bind();
    shader->setFloat(
        "clusterNear",
        sponge::platform::opengl::scene::ClusteredLights::clusterNear);
    shader->setFloat("farPlane", camera->getFar());
    shader->setFloat2("screenSize",
                      glm::vec2(static_cast<float>(w), static_cast<float>(h)));
    shader->unbind();

    setNumLights(numLights);
}

void MazeLayer::onDetach() {
    if (depthPrepassTexture != 0) {
        glDeleteTextures(1, &depthPrepassTexture);
        depthPrepassTexture = 0;
    }
    if (depthPrepassFbo != 0) {
        glDeleteFramebuffers(1, &depthPrepassFbo);
        depthPrepassFbo = 0;
    }
}

void MazeLayer::onEvent(Event& event) {
    sponge::event::EventDispatcher dispatcher(event);

    dispatcher.dispatch<MouseButtonPressedEvent>(
        [this](const MouseButtonPressedEvent& mbEvent) {
            return isActive() ? this->onMouseButtonPressed(mbEvent) : false;
        });
    dispatcher.dispatch<MouseButtonReleasedEvent>(
        [this](const MouseButtonReleasedEvent& mrEvent) {
            return isActive() ? this->onMouseButtonReleased(mrEvent) : false;
        });
    dispatcher.dispatch<MouseScrolledEvent>(
        [this](const MouseScrolledEvent& msEvent) {
            return isActive() ? this->onMouseScrolled(msEvent) : false;
        });
    dispatcher.dispatch<WindowFocusEvent>(
        [this](const WindowFocusEvent& wfEvent) {
            if (isActive()) {
                this->onWindowFocus(wfEvent);
            }
            return false;
        });
    dispatcher.dispatch<WindowResizeEvent>(
        [this](const WindowResizeEvent& wsEvent) {
            return this->onWindowResize(wsEvent);
        });
}

bool MazeLayer::onUpdate(const double elapsedTime) {
    // Update thread only — no GL calls.
    auto&      inputManager  = Application::get().getInputManager();
    const bool overlayActive = Maze::get().getExitLayer()->isActive() ||
                               Maze::get().getOptionLayer()->isActive();
    if (!overlayActive) {
        inputManager.setActiveContext(sponge::input::InputContext::Gameplay);
    }
    const InputSnapshot& snap = inputManager.getSnapshot();

    if (Application::get().isEventHandledByImGui()) {
        mouseButtonPressed = false;
    } else {
        if (!overlayActive && snap.isActive(GameAction::Pause)) {
            Maze::get().getExitLayer()->setActive(true);
            Application::get().setMouseVisible(true);
            inputManager.setMouseLookActive(false);
            mouseButtonPressed = false;
            inputManager.setActiveContext(sponge::input::InputContext::Menu);
#ifdef ENABLE_IMGUI
            if (isImguiOpen) {
                Maze::get().getImGuiLayer()->setActive(false);
            }
#endif
        }

#ifdef ENABLE_IMGUI
        if (!overlayActive && snap.isActive(GameAction::ToggleDebugUI)) {
            isImguiOpen = !isImguiOpen;
            Maze::get().getImGuiLayer()->setActive(isImguiOpen);
        }
#endif
        if (!overlayActive && snap.isActive(GameAction::ToggleFullscreen)) {
            Application::get().toggleFullscreen();
        }

        if (!overlayActive) {
            updateCamera(snap, elapsedTime);
        }
    }

    // Write the snapshot to the slot not being read by the render thread.
    const uint32_t readSlot  = renderReadIndex.load(std::memory_order_relaxed);
    const uint32_t writeSlot = (readSlot + 1) % 2;
    captureRenderFrame(writeSlot);

    return true;
}

void MazeLayer::captureRenderFrame(const uint32_t slotIndex) {
    auto& frame = renderFrames[slotIndex];

    frame.cameraMVP        = camera->getMVP();
    frame.cameraView       = camera->getViewMatrix();
    frame.cameraProjection = camera->getProjectionMatrix();
    frame.cameraPos        = camera->getPosition();
    frame.nearPlane        = camera->getNear();
    frame.farPlane         = camera->getFar();
    frame.screenWidth      = screenWidth;
    frame.screenHeight     = screenHeight;

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

    frame.fxaaEnabled    = fxaaEnabled;
    frame.bloomEnabled   = bloomEnabled;
    frame.bloomThreshold = bloomThreshold;
    frame.bloomIntensity = bloomIntensity;

    // Static after onAttach(); safe to copy.
    frame.objectModelMatrices = objectModelMatrices;
    frame.objectEmissives     = objectEmissives;
    frame.objectModels        = objectModels;

    // Publish slot; release/acquire pair with onRender()'s load.
    renderReadIndex.store(slotIndex, std::memory_order_release);
}

void MazeLayer::onRender() {
    // Render thread only — all GL calls here.
    if (pendingShadowRebuild.load(std::memory_order_acquire)) {
        const auto res =
            pendingShadowRebuildRes.load(std::memory_order_relaxed);
        shadowMap = std::make_unique<ShadowMap>(res);
        pendingShadowRebuild.store(false, std::memory_order_relaxed);
    }

    if (pendingResize.load(std::memory_order_acquire)) {
        const auto dims =
            pendingResizeDimensions.load(std::memory_order_relaxed);
        const auto w = static_cast<uint32_t>(dims >> 32U);
        const auto h = static_cast<uint32_t>(dims & 0xFFFFFFFFU);
        glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));
        if (fxaa) {
            fxaa->resize(w, h);
        }
        if (bloom) {
            bloom->resize(w, h);
        }
        screenWidth  = static_cast<int32_t>(w);
        screenHeight = static_cast<int32_t>(h);
        createDepthPrepassFbo(static_cast<int>(w), static_cast<int>(h));

        const auto shader = Mesh::getShader();
        shader->bind();
        shader->setFloat2("screenSize", glm::vec2(static_cast<float>(w),
                                                  static_cast<float>(h)));
        shader->unbind();
        pendingResize.store(false, std::memory_order_relaxed);
    }

    // Read the latest snapshot from the update thread.
    const auto& frame =
        renderFrames[renderReadIndex.load(std::memory_order_acquire)];

    // Phase 1: shadow map
    if (frame.shadowEnabled && frame.shadowCastShadow) {
        renderSceneToDepthMap(frame);
    }

    // Phase 2: depth prepass (also marks active volume tiles)
    if (clusteredLights) {
        clusteredLights->prepare(frame.cameraProjection);
    }
    renderDepthPrepass(frame);

    // Phase 3: light culling
    if (clusteredLights && frame.numLights > 0) {
        clusteredLights->update(frame.lightPositions.data(),
                                frame.lightColors.data(),
                                frame.lightAttenuationIndices.data(),
                                frame.numLights, frame.cameraView);
    }

    // Phase 4: opaque pass
    const bool hasFbo = (frame.bloomEnabled && bloom != nullptr) ||
                        (frame.fxaaEnabled && fxaa != nullptr);
    if (frame.bloomEnabled && bloom) {
        bloom->begin();
    } else if (frame.fxaaEnabled && fxaa) {
        fxaa->begin();
    }

    if (hasFbo) {
        // Blit prepass depth into the post-processing FBO so the opaque pass
        // can use GL_LEQUAL (zero overdraw). Both FBOs use
        // GL_DEPTH_COMPONENT24.
        blitDepthToCurrentFbo(frame.screenWidth, frame.screenHeight);
        glClear(GL_COLOR_BUFFER_BIT);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
    } else {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    renderGameObjects(frame);

    if (hasFbo) {
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
    }

    // Transparent pass (placeholder)
    renderLightCubes(frame);

    if (frame.bloomEnabled && bloom) {
        bloom->end();
        bloom->process(frame.bloomThreshold);
        if (frame.fxaaEnabled && fxaa) {
            fxaa->applyWithBloom(bloom->getSceneTexture(),
                                 bloom->getBloomTexture(),
                                 frame.bloomIntensity);
        } else {
            bloom->apply(frame.bloomIntensity);
        }
    } else if (frame.fxaaEnabled && fxaa) {
        fxaa->end();
        fxaa->apply();
    }

    if (hasFbo) {
        glDepthFunc(GL_LEQUAL);
    }
}

float MazeLayer::getAmbientOcclusion() const {
    return ao;
}

void MazeLayer::setAmbientOcclusion(const float val) {
    ao = val;

    const auto shader = Mesh::getShader();
    shader->bind();
    shader->setFloat("ao", ao);
    shader->unbind();
}

float MazeLayer::getAmbientStrength() const {
    return ambientStrength;
}

void MazeLayer::setAmbientStrength(const float val) {
    ambientStrength = val;

    const auto shader = Mesh::getShader();
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

    const auto shader = Mesh::getShader();
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

    const auto shader = Mesh::getShader();
    shader->bind();
    shader->setFloat3("directionalLight.color", directionalLight.color);
    shader->unbind();
}

glm::vec3 MazeLayer::getDirectionalLightDirection() const {
    return directionalLight.direction;
}

void MazeLayer::setDirectionalLightDirection(const glm::vec3& direction) {
    directionalLight.direction = direction;

    const auto shader = Mesh::getShader();
    shader->bind();
    shader->setFloat3("directionalLight.direction", directionalLight.direction);
    shader->unbind();
}

bool MazeLayer::getDirectionalLightEnabled() const {
    return directionalLight.enabled;
}

void MazeLayer::setDirectionalLightEnabled(const bool value) {
    directionalLight.enabled = value;

    const auto shader = Mesh::getShader();
    shader->bind();
    shader->setBoolean("directionalLight.enabled", directionalLight.enabled);
    shader->unbind();
}

uint32_t MazeLayer::getDirectionalLightShadowMapRes() const {
    return directionalLight.shadowMapRes;
}

void MazeLayer::setShadowMapRes(const uint32_t res) {
    directionalLight.shadowMapRes = res;
    pendingShadowRebuildRes.store(res, std::memory_order_relaxed);
    pendingShadowRebuild.store(true, std::memory_order_release);
}

bool MazeLayer::isMetallic() const {
    return metallic;
}

void MazeLayer::setMetallic(const bool val) {
    metallic = val;

    const auto shader = Mesh::getShader();
    shader->bind();
    shader->setFloat("metallic", metallic ? 1.F : 0.F);
    shader->unbind();
}

int32_t MazeLayer::getNumLights() const {
    return numLights;
}

void MazeLayer::setNumLights(const int32_t val) {
    numLights = val;

    std::mt19937                          rng(42U);
    std::uniform_real_distribution<float> jitterAngle(-0.4F, 0.4F);
    std::uniform_real_distribution<float> jitterRadius(-0.5F, 0.5F);

    for (int32_t i = 0; i < numLights; i++) {
        const float t =
            numLights > 1 ? static_cast<float>(i) / (numLights - 1) : 0.F;
        const float radius = 1.5F + t * 13.5F + jitterRadius(rng);
        const float angle =
            glm::two_pi<float>() * i / numLights + jitterAngle(rng);
        auto& light = pointLights.at(i);
        light.color = glm::vec3(1.F);
        light.position =
            glm::vec3(rotate(glm::mat4(1.F), angle, glm::vec3(0.F, 1.F, 0.F)) *
                      glm::vec4(0.F, 2.75F, -radius, 1.F));
        light.attenuationIndex = attenuationIndex;
    }

    const auto shader = Mesh::getShader();
    shader->bind();
    shader->setInteger("numLights", numLights);
    shader->unbind();
}

float MazeLayer::getRoughness() const {
    return roughness;
}

void MazeLayer::setRoughness(const float val) {
    roughness = val;

    const auto shader = Mesh::getShader();
    shader->bind();
    shader->setFloat("roughness", roughness);
    shader->unbind();
}

uint32_t MazeLayer::getShadowMapTextureId() const {
    return shadowMap->getDepthMapTextureId();
}

void MazeLayer::onWindowFocus(const WindowFocusEvent& event) {
    if (!event.isFocused()) {
        mouseButtonPressed = false;
        Application::get().setMouseVisible(true);
        Application::get().getInputManager().setMouseLookActive(false);
    }
}

bool MazeLayer::onMouseButtonPressed(const MouseButtonPressedEvent& event) {
    if (event.getMouseButton() == sponge::input::MouseButton::Button0) {
        Application::get().centerMouse();
        Application::get().setMouseVisible(false);
        Application::get().getInputManager().setMouseLookActive(true);
        mouseButtonPressed = true;
        return true;
    }
    return false;
}

bool MazeLayer::onMouseButtonReleased(const MouseButtonReleasedEvent& event) {
    if (event.getMouseButton() == sponge::input::MouseButton::Button0) {
        Application::get().setMouseVisible(true);
        Application::get().getInputManager().setMouseLookActive(false);
        mouseButtonPressed = false;
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
    queueResize(event.getWidth(), event.getHeight());
    return false;
}

void MazeLayer::queueResize(const uint32_t w, const uint32_t h) const {
    // Defer GL viewport/FXAA resize to onRender() on the render thread.
    // Store dimensions before setting the flag so the release fence on
    // pendingResize makes the relaxed store visible to the acquire reader.
    pendingResizeDimensions.store((static_cast<uint64_t>(w) << 32U) |
                                      static_cast<uint64_t>(h),
                                  std::memory_order_relaxed);
    pendingResize.store(true, std::memory_order_release);
}

void MazeLayer::renderGameObjects(const thread::MazeRenderFrame& frame) const {
    const auto shader = Mesh::getShader();
    shader->bind();
    if (clusteredLights) {
        clusteredLights->bindSSBOs();
    }
    shader->setFloat2("screenSize",
                      glm::vec2(static_cast<float>(frame.screenWidth),
                                static_cast<float>(frame.screenHeight)));
    shader->setFloat3("viewPos", frame.cameraPos);
    // World-space camera forward = -(third row of the view matrix).
    shader->setFloat3("viewForward",
                      -glm::vec3(frame.cameraView[0][2], frame.cameraView[1][2],
                                 frame.cameraView[2][2]));
    shader->setInteger("tilesZ",
                       clusteredLights ? clusteredLights->getTilesZ() : 1);

    if (frame.shadowEnabled && frame.shadowCastShadow) {
        shader->setMat4("lightSpaceMatrix", frame.lightSpaceMatrix);
        shadowMap->activateAndBindShadowTexture(1);
    }

    for (size_t i = 0; i < frame.objectModels.size(); i++) {
        const auto& modelMatrix = frame.objectModelMatrices[i];

        shader->setMat4("mvp", frame.cameraMVP * modelMatrix);
        shader->setMat4("model", modelMatrix);
        const auto normalMatrix =
            glm::mat4(glm::transpose(glm::inverse(glm::mat3(modelMatrix))));
        shader->setMat4("normalMatrix", normalMatrix);
        shader->setFloat3("emissive", frame.objectEmissives[i]);

        frame.objectModels[i]->render(shader);
    }

    shader->unbind();
}

void MazeLayer::createDepthPrepassFbo(const int w, const int h) {
    if (depthPrepassTexture != 0) {
        glDeleteTextures(1, &depthPrepassTexture);
    }
    if (depthPrepassFbo != 0) {
        glDeleteFramebuffers(1, &depthPrepassFbo);
    }

    glGenTextures(1, &depthPrepassTexture);
    glBindTexture(GL_TEXTURE_2D, depthPrepassTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
                 static_cast<GLsizei>(w), static_cast<GLsizei>(h), 0,
                 GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &depthPrepassFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, depthPrepassFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           depthPrepassTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MazeLayer::renderDepthPrepass(const thread::MazeRenderFrame& frame) const {
    glBindFramebuffer(GL_FRAMEBUFFER, depthPrepassFbo);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glClear(GL_DEPTH_BUFFER_BIT);

    depthPrepassShader->bind();
    depthPrepassShader->setFloat2(
        "screenSize", glm::vec2(static_cast<float>(frame.screenWidth),
                                static_cast<float>(frame.screenHeight)));
    depthPrepassShader->setFloat(
        "clusterNear",
        sponge::platform::opengl::scene::ClusteredLights::clusterNear);
    depthPrepassShader->setFloat("clusterFar", frame.farPlane);
    depthPrepassShader->setInteger(
        "tilesZ", clusteredLights ? clusteredLights->getTilesZ() : 1);
    for (size_t i = 0; i < frame.objectModels.size(); ++i) {
        depthPrepassShader->setMat4("mvp", frame.cameraMVP *
                                               frame.objectModelMatrices[i]);
        depthPrepassShader->setMat4(
            "modelView", frame.cameraView * frame.objectModelMatrices[i]);
        frame.objectModels[i]->render(depthPrepassShader);
    }
    depthPrepassShader->unbind();

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MazeLayer::blitDepthToCurrentFbo(const int w, const int h) const {
    GLint drawFbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, depthPrepassFbo);
    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(drawFbo));
}

void MazeLayer::renderLightCubes(const thread::MazeRenderFrame& frame) const {
    if (frame.numLights == 0) {
        return;
    }

    const auto shader = cube->getShader();
    shader->bind();

    for (int32_t i = 0; i < frame.numLights; i++) {
        shader->setFloat3("lightColor", frame.lightColors[i]);
        shader->setMat4(
            "mvp", scale(translate(frame.cameraMVP, frame.lightPositions[i]),
                         cubeScale));
        cube->render();
    }

    shader->unbind();
}

void MazeLayer::renderSceneToDepthMap(
    const thread::MazeRenderFrame& frame) const {
    shadowMap->bind();

    const auto shader = shadowMap->getShader();
    shader->bind();
    shader->setMat4("lightSpaceMatrix", frame.lightSpaceMatrix);

    for (size_t i = 0; i < frame.objectModels.size(); i++) {
        shader->setMat4("model", frame.objectModelMatrices[i]);
        frame.objectModels[i]->render(shader);
    }

    shader->unbind();

    shadowMap->unbind();
}

void MazeLayer::updateCamera(const InputSnapshot& snap,
                             const double         elapsedTime) const {
    camera->moveForward(elapsedTime * static_cast<double>(snap.getAxis(
                                          GameAction::MoveForward)));
    camera->moveBackward(
        elapsedTime * static_cast<double>(snap.getAxis(GameAction::MoveBack)));
    camera->strafeLeft(elapsedTime *
                       static_cast<double>(snap.getAxis(GameAction::MoveLeft)));
    camera->strafeRight(
        elapsedTime * static_cast<double>(snap.getAxis(GameAction::MoveRight)));

    // Apply mouse look only when the mouse is captured (left button held),
    // or always for gamepad look (right stick).
    const bool useMouse =
        mouseButtonPressed &&
        snap.activeDevice == sponge::input::ActiveDevice::KeyboardMouse;
    const bool useGamepad =
        snap.activeDevice == sponge::input::ActiveDevice::Gamepad;

    if (useMouse || useGamepad) {
        const float lookH = snap.getAxis(GameAction::LookHorizontal);
        const float lookV = snap.getAxis(GameAction::LookVertical);
        if (lookH != 0.F || lookV != 0.F) {
            camera->mouseMove({ lookH, lookV });
        }
    }
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

bool MazeLayer::isBloomEnabled() const {
    return bloomEnabled;
}

void MazeLayer::setBloomEnabled(const bool val) {
    bloomEnabled = val;
}

float MazeLayer::getBloomThreshold() const {
    return bloomThreshold;
}

void MazeLayer::setBloomThreshold(const float val) {
    bloomThreshold = val;
}

float MazeLayer::getBloomIntensity() const {
    return bloomIntensity;
}

void MazeLayer::setBloomIntensity(const float val) {
    bloomIntensity = val;
}

#ifdef ENABLE_IMGUI
bool MazeLayer::isImguiActive() const {
    return isImguiOpen;
}
#endif
}  // namespace game::layer
