#include "layer/mazelayer.hpp"

#include "core/settings.hpp"
#include "input/gameaction.hpp"
#include "input/inputcontext.hpp"
#include "input/mousecode.hpp"
#include "maze.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"
#include "platform/opengl/scene/mesh.hpp"
#include "resourcemanager.hpp"
#include "scene/light.hpp"

#include <glm/ext/matrix_transform.hpp>

#include <array>
#include <memory>
#include <mutex>
#include <random>
#include <string>

namespace {
constexpr auto cameraPosition = glm::vec3(-5.F, 1.5F, 0.F);

constexpr auto dirLightCastsShadow = true;
constexpr auto dirLightColor       = glm::vec3(1.F, 1.F, 1.F);
constexpr auto dirLightDirection   = glm::vec3(0.F, -20.F, 1.333F);
constexpr auto dirLightEnabled     = true;
constexpr auto defaultShadowMapRes = 1024U;

constexpr auto cubeScale = glm::vec3(.1F);

constexpr std::string_view cameraName = "maze";

constexpr int32_t maxPointLights =
    sponge::platform::opengl::scene::ClusteredLights::maxLights;

game::scene::DirectionalLight                       directionalLight;
std::array<game::scene::PointLight, maxPointLights> pointLights;

using game::layer::GameObject;

std::array gameObjects = {
    // GameObject{ .name  = "floor",
    //             .path  = "/models/gltf/floor/floor.glb",
    //             .scale = glm::vec3(2.F) },

    // GameObject{ .name = "cube1",
    //             .path = "/models/gltf/cube/cube-tex.glb",
    //             .scale = glm::vec3(1.F),
    //             .rotation    = { .angle = 0.F, .axis{ 0.F, 1.F, 0.F }, },
    //             .translation = glm::vec3(-1.5F, .85F, -.5F), },
    //
    // GameObject{ .name = "cube2",
    //             .path = "/models/gltf/cube/cube-tex.glb",
    //             .scale = glm::vec3(.5F),
    //             .rotation    = { .angle = 0.F, .axis{ 0.F, 1.F, 0.F }, },
    //             .translation = glm::vec3(0.F, 0.F, .5F), },

    // GameObject{ .name        = "cube3",
    //             .path        = "/models/gltf/cube/cube-tex.glb",
    //             .scale       = glm::vec3(.25F),
    //             .rotation    = { .angle = glm::radians(60.F),
    //                              .axis  = glm::vec3(1.F, 0.F, 1.F), },
    //             .translation = glm::vec3(-1.F, 2.25F, 1.F),
    //             .emissive    = glm::vec3(1.5F, 1.2F, 0.5F), },

    GameObject{ .name        = "helmet",
                .path        = "/models/gltf/helmet/DamagedHelmet.glb",
                .scale       = glm::vec3(.3F),
                .rotation    = { .angle = glm::radians(-75.F),
                                 .axis  = glm::vec3(0.F, 1.F, 0.F), },
                .translation = glm::vec3(2.F, 0.5F, 0.F), },

    GameObject{ .name        = "sponza",
                .path        = "/models/gltf/sponza/sponza.glb",
                .translation = glm::vec3(0.F, 0.F, 0.F), },
};
}  // namespace

namespace game::layer {
using game::thread::AntiAliasing;
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
using sponge::platform::opengl::renderer::createRenderTarget;
using sponge::platform::opengl::scene::Bloom;
using sponge::platform::opengl::scene::ClusteredLights;
using sponge::platform::opengl::scene::Cube;
using sponge::platform::opengl::scene::FXAA;
using sponge::platform::opengl::scene::Mesh;
using sponge::platform::opengl::scene::Model;
using sponge::platform::opengl::scene::ModelCreateInfo;
using sponge::platform::opengl::scene::SceneTarget;
using sponge::platform::opengl::scene::ShadowMap;
using sponge::platform::opengl::scene::TAA;

MazeLayer::MazeLayer() : Layer("maze") {}

std::vector<ModelCreateInfo> MazeLayer::getModelLoadRequests() const {
    std::vector<ModelCreateInfo> requests;
    requests.reserve(gameObjects.size());
    for (auto& gameObject : gameObjects) {
        requests.push_back({
            .name = std::string(gameObject.name),
            .path = std::string(gameObject.path),
        });
    }
    return requests;
}

void MazeLayer::finishLoading(std::vector<std::shared_ptr<Model>> builtModels) {
    for (auto& gameObject : gameObjects) {
        // model matrix never changes after this point
        objectModelMatrices.push_back(glm::scale(
            glm::rotate(glm::translate(glm::mat4(1.0f), gameObject.translation),
                        gameObject.rotation.angle, gameObject.rotation.axis),
            gameObject.scale));
        objectEmissives.push_back(gameObject.emissive);
    }
    objectModels = std::move(builtModels);

    const auto gameCameraCreateInfo =
        scene::GameCameraCreateInfo{ .name = std::string(cameraName) };
    camera = ResourceManager::createGameCamera(gameCameraCreateInfo);
    camera->setViewportSize(Maze::get().getWindow()->getWidth(),
                            Maze::get().getWindow()->getHeight());
    camera->setPosition(cameraPosition);

    const auto shader = Mesh::getShader();
    shader->bind();

    shader->setFloat("ao", ao);

    shader->setFloat("ambientStrength", ambientStrength);

    const auto savedShadowRes = sponge::core::Settings::getUInt32(
        "video.shadowRes", defaultShadowMapRes);

    directionalLight = {
        .enabled      = dirLightEnabled,
        .castShadow   = dirLightCastsShadow,
        .color        = dirLightColor,
        .direction    = dirLightDirection,
        .shadowMapRes = savedShadowRes,
    };

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

    taa = std::make_unique<TAA>(Maze::get().getWindow()->getWidth(),
                                Maze::get().getWindow()->getHeight());

    bloom = std::make_unique<Bloom>(Maze::get().getWindow()->getWidth(),
                                    Maze::get().getWindow()->getHeight());

    sceneTarget =
        std::make_unique<SceneTarget>(Maze::get().getWindow()->getWidth(),
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
    });
    createDepthPrepassFbo(w, h);

    shader->bind();
    shader->setFloat("clusterNear", ClusteredLights::clusterNear);
    shader->setFloat("farPlane", camera->getFar());
    shader->setFloat2("screenSize",
                      glm::vec2(static_cast<float>(w), static_cast<float>(h)));
    shader->unbind();

    setNumLights(numLights);

    // Static after this: bake into both snapshot slots once, not per frame.
    // prevObjectModelMatrices is the same data because nothing animates; if a
    // model matrix ever becomes per-frame, captureRenderFrame() must write
    // BOTH arrays — current from this frame, previous from the last — or TAA
    // measures motion between a live matrix and a stale one.
    for (auto& frame : renderFrames) {
        frame.objectModelMatrices     = objectModelMatrices;
        frame.prevObjectModelMatrices = objectModelMatrices;
        frame.objectEmissives         = objectEmissives;
        frame.objectModels            = objectModels;
    }

    // must precede setActive(true) in activate(): onUpdate/onRender only run
    // while isActive()
    resourcesReady.store(true, std::memory_order_release);
    activate();
}

void MazeLayer::activate() {
#ifdef ENABLE_IMGUI
    if (isImguiOpen) {
        Maze::get().getImGuiLayer()->setActive(true);
    }
#endif
    setActive(true);
}

void MazeLayer::onDetach() {
    if (depthPrepassTexture != 0) {
        glDeleteTextures(1, &depthPrepassTexture);
        depthPrepassTexture = 0;
    }
    if (velocityTexture != 0) {
        glDeleteTextures(1, &velocityTexture);
        velocityTexture = 0;
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
    if (!resourcesReady.load(std::memory_order_acquire)) {
        return true;
    }

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
            Application::get().requestMouseVisible(true);
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

    // One locked read for the whole snapshot: the jitter branch below and
    // frame.antiAliasing have to agree. Reading the member twice lets a mode
    // switch land between them and tag a jittered frame as FXAA.
    AntiAliasing aaMode;
    {
        std::scoped_lock lock(settingsMutex);
        aaMode = antiAliasing;
    }

    frame.cameraMVP      = camera->getMVP();
    frame.cameraViewProj = frame.cameraMVP;
    if (aaMode == AntiAliasing::Taa) {
        const auto jitter = TAA::haltonJitter(
            jitterIndex++, static_cast<uint32_t>(screenWidth.load()),
            static_cast<uint32_t>(screenHeight.load()));
        frame.cameraMVP =
            glm::translate(glm::mat4(1.F), glm::vec3(jitter, 0.F)) *
            frame.cameraMVP;
    }
    frame.invCameraMVP       = glm::inverse(frame.cameraMVP);
    frame.prevCameraViewProj = prevCameraViewProj;
    prevCameraViewProj       = frame.cameraViewProj;

    frame.cameraView       = camera->getViewMatrix();
    frame.cameraProjection = camera->getProjectionMatrix();
    frame.cameraPos        = camera->getPosition();
    frame.nearPlane        = camera->getNear();
    frame.farPlane         = camera->getFar();
    frame.screenWidth      = screenWidth;
    frame.screenHeight     = screenHeight;

    {
        // ImGui setters mutate these from the render thread.
        std::scoped_lock lock(settingsMutex);

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

        frame.numLights             = numLights;
        frame.lightAttenuationIndex = attenuationIndex;
        for (int32_t i = 0; i < numLights; i++) {
            frame.lightPositions[i]     = pointLights.at(i).position;
            frame.prevLightPositions[i] = prevLightPositions.at(i);
            frame.lightColors[i]        = pointLights.at(i).color;
            prevLightPositions.at(i)    = pointLights.at(i).position;
        }

        frame.antiAliasing   = aaMode;
        frame.bloomEnabled   = bloomEnabled;
        frame.bloomThreshold = bloomThreshold;
        frame.bloomIntensity = bloomIntensity;
    }

    // Publication happens in onFrameSync() on the main thread, while both
    // workers are idle — publishing here would race the in-flight render and
    // make the frame pairing nondeterministic.
    writtenSlot = slotIndex;
}

void MazeLayer::onFrameSync() {
    // Release pairs with onRender()'s acquire load.
    renderReadIndex.store(writtenSlot, std::memory_order_release);
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
        if (taa) {
            taa->resize(w, h);
        }
        if (bloom) {
            bloom->resize(w, h);
        }
        sceneTarget->resize(w, h);
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

    // Phase 2: depth prepass
    renderDepthPrepass(frame);

    // Phase 3: light culling
    if (clusteredLights && frame.numLights > 0) {
        clusteredLights->update(frame.lightPositions.data(),
                                frame.lightColors.data(),
                                frame.lightAttenuationIndex, frame.numLights,
                                frame.cameraView, frame.cameraProjection);
    }

    // Phase 4: opaque pass, into the linear HDR scene target.
    const bool fxaaActive = frame.antiAliasing == AntiAliasing::Fxaa && fxaa;
    const bool taaActive  = frame.antiAliasing == AntiAliasing::Taa && taa;

    sceneTarget->begin();

    // Blit prepass depth in so the opaque pass can use GL_LEQUAL (zero
    // overdraw). Both FBOs use GL_DEPTH_COMPONENT24.
    blitDepthToCurrentFbo(frame.screenWidth, frame.screenHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    renderGameObjects(frame);

    // The cubes are in the prepass too, so their depth is already in the
    // buffer — GL_LESS would reject every one of their fragments. They stay
    // on the opaque pass's GL_LEQUAL / depth-write-off state.
    renderLightCubes(frame);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    sceneTarget->end();

    // Phase 5: bloom, extracted from linear radiance rather than from an
    // already tone-mapped image.
    // Named apart from the bloomIntensity member on purpose: that one is the
    // ImGui-mutated setting behind settingsMutex, and the render thread must
    // only ever see it through the snapshot.
    uint32_t bloomTexId  = 0;
    float    bloomWeight = 0.F;
    if (frame.bloomEnabled && bloom) {
        bloom->process(sceneTarget->getTexture(), frame.bloomThreshold);
        bloomTexId  = bloom->getBloomTexture();
        bloomWeight = frame.bloomIntensity;
    }

    // Phase 6: resolve to display. Anti-aliasing, when on, consumes the
    // resolved image and does its own dithered write to the back buffer.
    if (fxaaActive) {
        fxaa->begin();
    } else if (taaActive) {
        taa->begin();
    }

    sceneTarget->resolve(bloomTexId, bloomWeight, !fxaaActive && !taaActive);

    if (fxaaActive) {
        fxaa->end();
        fxaa->apply();
    } else if (taaActive) {
        // ponytail: TAA accumulates the tone-mapped image, which is where it
        // already ran. Accumulating in linear with a reversible weighting curve
        // resolves highlight edges better; that is a change to TAA, not to the
        // pass order this commit is fixing.
        taa->end();
        taa->apply(depthPrepassTexture, velocityTexture, frame.invCameraMVP,
                   frame.prevCameraViewProj);
    }

    glDepthFunc(GL_LEQUAL);
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
    {
        std::scoped_lock lock(settingsMutex);
        attenuationIndex = val;
    }
    setNumLights(numLights);
}

std::shared_ptr<scene::GameCamera> MazeLayer::getCamera() const {
    return camera;
}

bool MazeLayer::getDirectionalLightCastsShadow() const {
    return directionalLight.castShadow;
}

void MazeLayer::setDirectionalLightCastsShadow(const bool value) {
    {
        std::scoped_lock lock(settingsMutex);
        directionalLight.castShadow = value;
    }

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
    {
        std::scoped_lock lock(settingsMutex);
        directionalLight.direction = direction;
    }

    const auto shader = Mesh::getShader();
    shader->bind();
    shader->setFloat3("directionalLight.direction", directionalLight.direction);
    shader->unbind();
}

bool MazeLayer::getDirectionalLightEnabled() const {
    return directionalLight.enabled;
}

void MazeLayer::setDirectionalLightEnabled(const bool value) {
    {
        std::scoped_lock lock(settingsMutex);
        directionalLight.enabled = value;
    }

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

int32_t MazeLayer::getNumLights() const {
    return numLights;
}

void MazeLayer::setNumLights(const int32_t val) {
    {
        std::scoped_lock lock(settingsMutex);
        numLights = std::clamp(val, 0, maxPointLights);

        std::mt19937                   rng(42U);
        std::uniform_real_distribution jitterAngle(-0.4F, 0.4F);
        std::uniform_real_distribution jitterRadius(-0.5F, 0.5F);

        for (int32_t i = 0; i < numLights; i++) {
            const float t =
                numLights > 1 ? static_cast<float>(i) / (numLights - 1) : 0.F;
            const float radius = 1.5F + t * 13.5F + jitterRadius(rng);
            const float angle =
                glm::two_pi<float>() * i / numLights + jitterAngle(rng);
            auto& light    = pointLights.at(i);
            light.color    = glm::vec3(1.F);
            light.position = glm::vec3(
                rotate(glm::mat4(1.F), angle, glm::vec3(0.F, 1.F, 0.F)) *
                glm::vec4(0.F, 5.75F, -radius, 1.F));
        }
    }

    const auto shader = Mesh::getShader();
    shader->bind();
    shader->setInteger("numLights", numLights);
    shader->unbind();
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
    if (!camera) {
        // camera not created until finishLoading(); resize is moot before then
        return false;
    }
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
    shader->setInteger("attenuationIndex", frame.lightAttenuationIndex);

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
    if (velocityTexture != 0) {
        glDeleteTextures(1, &velocityTexture);
    }
    if (depthPrepassFbo != 0) {
        glDeleteFramebuffers(1, &depthPrepassFbo);
    }

    depthPrepassTexture = createRenderTarget(
        static_cast<uint32_t>(w), static_cast<uint32_t>(h),
        GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, GL_NEAREST);

    // RG16F holds a UV delta, not an absolute UV: at 1920 wide a one-pixel
    // motion is ~5e-4, which a half float carries accurately as a delta and
    // would quantise to worse than a pixel as an absolute coordinate.
    velocityTexture =
        createRenderTarget(static_cast<uint32_t>(w), static_cast<uint32_t>(h),
                           GL_RG16F, GL_RG, GL_FLOAT, GL_NEAREST);

    glGenFramebuffers(1, &depthPrepassFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, depthPrepassFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           depthPrepassTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           velocityTexture, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SPONGE_GL_CRITICAL("Depth prepass framebuffer is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MazeLayer::renderDepthPrepass(const thread::MazeRenderFrame& frame) const {
    // Only TAA reads the velocity target, so the other modes mask the writes
    // off. They still carry the plumbing: the RG16F attachment stays
    // allocated and the prepass shader still computes both clip-space
    // varyings. Cheap, but not free — measure before assuming parity with the
    // old depth-only pass.
    const bool writeVelocity = frame.antiAliasing == AntiAliasing::Taa;

    glBindFramebuffer(GL_FRAMEBUFFER, depthPrepassFbo);
    glColorMask(writeVelocity ? GL_TRUE : GL_FALSE,
                writeVelocity ? GL_TRUE : GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glClear(GL_DEPTH_BUFFER_BIT);

    if (writeVelocity) {
        // Blending is enabled globally (RendererAPI) and never turned off, so
        // the restore below is unconditional. Without this, the shader writes
        // no alpha and the motion vectors get blended against the clear colour
        // and never reach the texture. Motion is not a colour; there is
        // nothing here to blend.
        glDisable(GL_BLEND);
        // Explicit zero rather than glClear, which would use the global grey
        // clear colour and read back as ~22 pixels of bogus motion.
        constexpr std::array noMotion = { 0.F, 0.F, 0.F, 0.F };
        glClearBufferfv(GL_COLOR, 0, noMotion.data());
    }

    depthPrepassShader->bind();
    for (size_t i = 0; i < frame.objectModels.size(); ++i) {
        // Rasterization uses the jittered matrix so prepass depth lines up with
        // the scene pass; motion is measured unjittered, or the jitter itself
        // would read as movement.
        depthPrepassShader->setMat4("mvp", frame.cameraMVP *
                                               frame.objectModelMatrices[i]);
        depthPrepassShader->setMat4(
            "mvpNoJitter", frame.cameraViewProj * frame.objectModelMatrices[i]);
        depthPrepassShader->setMat4("prevMvpNoJitter",
                                    frame.prevCameraViewProj *
                                        frame.prevObjectModelMatrices[i]);
        frame.objectModels[i]->render(depthPrepassShader);
    }

    // Light cubes go through the same shader: position-only geometry at
    // location 0, so they need no variant of their own. Including them here
    // is what gives them depth coverage and motion vectors.
    for (int32_t i = 0; i < frame.numLights; i++) {
        const auto model = scale(
            translate(glm::mat4(1.F), frame.lightPositions[i]), cubeScale);
        const auto prevModel = scale(
            translate(glm::mat4(1.F), frame.prevLightPositions[i]), cubeScale);
        depthPrepassShader->setMat4("mvp", frame.cameraMVP * model);
        depthPrepassShader->setMat4("mvpNoJitter",
                                    frame.cameraViewProj * model);
        depthPrepassShader->setMat4("prevMvpNoJitter",
                                    frame.prevCameraViewProj * prevModel);
        cube->render();
    }

    depthPrepassShader->unbind();

    if (writeVelocity) {
        glEnable(GL_BLEND);
    }
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

AntiAliasing MazeLayer::getAntiAliasing() const {
    return antiAliasing;
}

void MazeLayer::setAntiAliasing(const AntiAliasing val) {
    {
        std::scoped_lock lock(settingsMutex);
        antiAliasing = val;
    }
    if (taa) {
        // The accumulated history belongs to whatever was on screen before.
        taa->invalidateHistory();
    }
}

bool MazeLayer::isBloomEnabled() const {
    return bloomEnabled;
}

void MazeLayer::setBloomEnabled(const bool val) {
    std::scoped_lock lock(settingsMutex);
    bloomEnabled = val;
}

float MazeLayer::getBloomThreshold() const {
    return bloomThreshold;
}

void MazeLayer::setBloomThreshold(const float val) {
    std::scoped_lock lock(settingsMutex);
    bloomThreshold = val;
}

float MazeLayer::getBloomIntensity() const {
    return bloomIntensity;
}

void MazeLayer::setBloomIntensity(const float val) {
    std::scoped_lock lock(settingsMutex);
    bloomIntensity = val;
}

#ifdef ENABLE_IMGUI
bool MazeLayer::isImguiActive() const {
    return isImguiOpen;
}
#endif
}  // namespace game::layer
