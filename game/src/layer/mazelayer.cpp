#include "layer/mazelayer.hpp"

#include "core/settings.hpp"
#include "debug/profiler.hpp"
#include "input/gameaction.hpp"
#include "input/inputcontext.hpp"
#include "input/mousecode.hpp"
#include "logging/log.hpp"
#include "maze.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/readback.hpp"
#include "platform/opengl/renderer/rendererapi.hpp"
#include "platform/opengl/scene/mesh.hpp"
#include "resourcemanager.hpp"
#include "scene/light.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <ranges>
#include <string>

namespace {
constexpr std::string_view cameraName = "maze";
constexpr std::string_view scenePath  = "/scenes/maze.yaml";

constexpr int32_t maxPointLights =
    sponge::platform::opengl::scene::ClusteredLights::maxLights;

game::scene::DirectionalLight                       directionalLight;
std::array<game::scene::PointLight, maxPointLights> pointLights;

glm::mat4 lightCubeModel(const glm::vec3& position,
                         const glm::vec3& cubeScale) {
    return glm::scale(glm::translate(glm::mat4(1.F), position), cubeScale);
}
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
using sponge::platform::opengl::renderer::DepthFunc;
using sponge::platform::opengl::renderer::readBackBuffer;
using sponge::platform::opengl::renderer::readTexture;
using sponge::platform::opengl::renderer::RendererAPI;
using sponge::platform::opengl::scene::Bloom;
using sponge::platform::opengl::scene::ClusteredLights;
using sponge::platform::opengl::scene::Cube;
using sponge::platform::opengl::scene::DepthPrepass;
using sponge::platform::opengl::scene::FXAA;
using sponge::platform::opengl::scene::Mesh;
using sponge::platform::opengl::scene::Model;
using sponge::platform::opengl::scene::ModelCreateInfo;
using sponge::platform::opengl::scene::OcclusionCuller;
using sponge::platform::opengl::scene::PlanarReflection;
using sponge::platform::opengl::scene::ReflectionProbe;
using sponge::platform::opengl::scene::SceneTarget;
using sponge::platform::opengl::scene::ShadowMap;
using sponge::platform::opengl::scene::Ssao;
using sponge::platform::opengl::scene::Ssr;
using sponge::platform::opengl::scene::TAA;
using thread::AntiAliasing;

MazeLayer::MazeLayer() :
    Layer("maze"), sceneDesc(scene::loadScene(std::string(scenePath))) {
    ambientStrength  = sceneDesc.lighting.ambient.strength;
    ao               = sceneDesc.lighting.ambient.ambientOcclusion;
    attenuationIndex = sceneDesc.lighting.point.attenuationIndex;
    numLights = std::clamp(sceneDesc.lighting.point.count, 0, maxPointLights);
    bloomThreshold = sceneDesc.bloom.threshold;
    bloomIntensity = sceneDesc.bloom.intensity;
}

std::vector<ModelCreateInfo> MazeLayer::getModelLoadRequests() const {
    std::vector<ModelCreateInfo> requests;
    requests.reserve(sceneDesc.objects.size());
    for (const auto& object : sceneDesc.objects) {
        // name = path, not object.name: AssetManager dedupes models on this
        // field, so two objects sharing a path must share a key too.
        requests.push_back({
            .name = object.path,
            .path = object.path,
        });
    }
    return requests;
}

void MazeLayer::finishLoading(std::vector<std::shared_ptr<Model>> builtModels) {
    for (const auto& object : sceneDesc.objects) {
        // model matrix never changes after this point
        objectModelMatrices.push_back(glm::scale(
            glm::rotate(glm::translate(glm::mat4(1.0f), object.translation),
                        object.rotation.angle, object.rotation.axis),
            object.scale));
        objectEmissives.push_back(object.emissive);
        objectRefraction.push_back(object.refraction);
        objectReflectivity.push_back(object.reflective);
        if (object.planar) {
            if (planarObject) {
                SPONGE_WARN("Scene object '{}' is a second planar mirror, it "
                            "uses SSR",
                            object.name);
            } else {
                planarObject = objectReflectivity.size() - 1;
            }
        }
    }
    objectModels = std::move(builtModels);

    objectMeshWorldBounds.reserve(objectModels.size());
    for (size_t i = 0; i < objectModels.size(); i++) {
        const auto& model       = objectModels[i];
        const auto& modelMatrix = objectModelMatrices[i];

        std::vector<sponge::scene::AABB> meshBounds;
        meshBounds.reserve(model->getMeshCount());
        sponge::scene::AABB objectBounds{
            glm::vec3(std::numeric_limits<float>::max()),
            glm::vec3(std::numeric_limits<float>::lowest())
        };
        for (size_t m = 0; m < model->getMeshCount(); m++) {
            const auto worldBounds =
                sponge::scene::transform(model->getMeshBounds(m), modelMatrix);
            objectBounds.min = glm::min(objectBounds.min, worldBounds.min);
            objectBounds.max = glm::max(objectBounds.max, worldBounds.max);
            meshBounds.push_back(worldBounds);
        }
        objectMeshWorldBounds.push_back(std::move(meshBounds));
        objectWorldBounds.push_back(objectBounds);
    }

    sceneBounds = { glm::vec3(std::numeric_limits<float>::max()),
                    glm::vec3(std::numeric_limits<float>::lowest()) };
    for (const auto& box : objectWorldBounds) {
        sceneBounds.min = glm::min(sceneBounds.min, box.min);
        sceneBounds.max = glm::max(sceneBounds.max, box.max);
    }

    const auto gameCameraCreateInfo =
        scene::GameCameraCreateInfo{ .name = std::string(cameraName) };
    camera = ResourceManager::createGameCamera(gameCameraCreateInfo);
    camera->setViewportSize(Maze::get().getWindow()->getWidth(),
                            Maze::get().getWindow()->getHeight());
    camera->setFov(sceneDesc.camera.fov);
    camera->setOrientation(sceneDesc.camera.yaw, sceneDesc.camera.pitch);
    camera->setPosition(sceneDesc.camera.position);

    const auto shader = Mesh::getShader();
    shader->setFloat("ao", ao);
    shader->setBoolean("ssaoEnabled", ssaoEnabled);

    shader->setFloat("ambientStrength", ambientStrength);

    // The scene file supplies the default; a resolution saved from the
    // options screen overrides it.
    const auto savedShadowRes = sponge::core::Settings::getUInt32(
        "video.shadowRes", sceneDesc.lighting.directional.shadowMapRes);

    directionalLight = {
        .enabled      = sceneDesc.lighting.directional.enabled,
        .castShadow   = sceneDesc.lighting.directional.castShadow,
        .color        = sceneDesc.lighting.directional.color,
        .direction    = sceneDesc.lighting.directional.direction,
        .shadowMapRes = savedShadowRes,
    };

    shader->setBoolean("directionalLight.enabled", directionalLight.enabled);
    shader->setBoolean("directionalLight.castShadow",
                       directionalLight.castShadow);
    shader->setFloat3("directionalLight.direction", directionalLight.direction);
    shader->setFloat3("directionalLight.color", directionalLight.color);
    shader->setFloat("evsmBleedThreshold", 0.2F);

    shadowMap = std::make_unique<ShadowMap>(directionalLight.shadowMapRes);
    cube      = std::make_unique<Cube>();
    occlusionCuller = std::make_unique<OcclusionCuller>(objectModels.size());
    shadowOcclusionCuller =
        std::make_unique<OcclusionCuller>(objectModels.size());

    fxaa = std::make_unique<FXAA>(Maze::get().getWindow()->getWidth(),
                                  Maze::get().getWindow()->getHeight());

    taa = std::make_unique<TAA>(Maze::get().getWindow()->getWidth(),
                                Maze::get().getWindow()->getHeight());

    bloom = std::make_unique<Bloom>(Maze::get().getWindow()->getWidth(),
                                    Maze::get().getWindow()->getHeight());

    ssao = std::make_unique<Ssao>(Maze::get().getWindow()->getWidth(),
                                  Maze::get().getWindow()->getHeight());

    ssr = std::make_unique<Ssr>();

    if (planarObject) {
        planarReflection = std::make_unique<PlanarReflection>(
            Maze::get().getWindow()->getWidth(),
            Maze::get().getWindow()->getHeight());
        planarShader = AssetManager::createShader({
            .name           = "planar",
            .vertexShader   = "planar.vert",
            .fragmentShader = "planar.frag",
        });
    }

    if (sceneDesc.probe) {
        probe = std::make_unique<ReflectionProbe>();
    }

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
        .name           = "depthprepass",
        .vertexShader   = "depthprepass.vert",
        .fragmentShader = "depthprepass.frag",
    });
    // Same stages, own program: the cube VAO enables only position, the
    // mesh VAOs enable four attributes, and NVIDIA recompiles the vertex
    // shader whenever one program sees both (API PERFORMANCE 131218).
    depthPrepassCubeShader = AssetManager::createShader({
        .name           = "depthprepass_cube",
        .vertexShader   = "depthprepass.vert",
        .fragmentShader = "depthprepass.frag",
    });
    refractionShader       = AssetManager::createShader({
        .name           = "refraction",
        .vertexShader   = "refraction.vert",
        .fragmentShader = "refraction.frag",
    });
    depthPrepass = std::make_unique<DepthPrepass>(static_cast<uint32_t>(w),
                                                  static_cast<uint32_t>(h));

    shader->setFloat("clusterNear", ClusteredLights::clusterNear);
    shader->setFloat("farPlane", camera->getFar());
    shader->setFloat2("screenSize",
                      glm::vec2(static_cast<float>(w), static_cast<float>(h)));

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
        frame.objectRefraction        = objectRefraction;
        frame.objectReflectivity      = objectReflectivity;
        frame.objectModels            = objectModels;

        // All visible until the first captureRenderFrame() runs its cull.
        frame.objectMeshVisible.resize(objectMeshWorldBounds.size());
        frame.objectMeshVisibleLight.resize(objectMeshWorldBounds.size());
        for (size_t i = 0; i < objectMeshWorldBounds.size(); i++) {
            frame.objectMeshVisible[i].assign(objectMeshWorldBounds[i].size(),
                                              1);
            frame.objectMeshVisibleLight[i].assign(
                objectMeshWorldBounds[i].size(), 1);
        }
    }

    // must precede setActive(true) in activate(): onUpdate/onRender only run
    // while isActive()
    resourcesReady.store(true, std::memory_order_release);
    activate();
}

void MazeLayer::activate() {
    if (isImguiOpen) {
        Maze::get().getImGuiLayer()->setActive(true);
    }
    setActive(true);
}

void MazeLayer::onDetach() {
    depthPrepass.reset();
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

    auto&      inputManager = Application::get().getInputManager();
    const bool overlayActive =
        Maze::get().getExitLayer()->isActive() || Maze::get().isOptionsOpen();
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
            if (isImguiOpen) {
                Maze::get().getImGuiLayer()->setActive(false);
            }
        }

        if (!overlayActive && snap.isActive(GameAction::ToggleDebugUI)) {
            isImguiOpen = !isImguiOpen;
            Maze::get().getImGuiLayer()->setActive(isImguiOpen);
        }
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
    const AntiAliasing aaMode = [this] {
        std::scoped_lock lock(settingsMutex);
        return antiAliasing;
    }();

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
        SPONGE_PROFILE_SECTION("frustum cull");
        const auto cullStart = std::chrono::steady_clock::now();

        // Unjittered: the frustum a jittered MVP implies is the same one, off
        // by a sub-pixel translation not worth the extra matrix.
        const sponge::scene::Frustum frustum(frame.cameraViewProj);

        uint32_t visible = 0;
        uint32_t total   = 0;
        for (size_t i = 0; i < objectMeshWorldBounds.size(); i++) {
            const auto& bounds  = objectMeshWorldBounds[i];
            auto&       visMask = frame.objectMeshVisible[i];
            for (size_t m = 0; m < bounds.size(); m++) {
                const bool vis = frustum.intersects(bounds[m]);
                visMask[m]     = vis ? 1 : 0;
                visible += vis ? 1U : 0U;
            }
            total += static_cast<uint32_t>(bounds.size());
        }
        visibleMeshCount.store(visible, std::memory_order_relaxed);
        totalMeshCount.store(total, std::memory_order_relaxed);

        const auto cullUs =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - cullStart)
                .count();
        cullMicros.store(static_cast<uint32_t>(cullUs),
                         std::memory_order_relaxed);
    }

    // Planar mirror: the camera reflected in the mirror's top face, with its
    // near plane moved onto the mirror so nothing below it is drawn. Skipped
    // when the mirror itself is frustum-culled, so a mirror behind the camera
    // costs nothing beyond the object it already is.
    frame.planarActive  = false;
    const bool planarOn = [this] {
        std::scoped_lock lock(settingsMutex);
        return planarEnabled;
    }();
    if (planarOn && planarObject &&
        std::ranges::any_of(frame.objectMeshVisible[*planarObject],
                            [](const uint8_t v) { return v != 0; })) {
        const auto&     model = objectModelMatrices[*planarObject];
        const glm::vec3 point(model * glm::vec4(0.F, 1.F, 0.F, 1.F));
        const glm::vec3 normal =
            glm::normalize(glm::transpose(glm::inverse(glm::mat3(model))) *
                           glm::vec3(0.F, 1.F, 0.F));
        const glm::vec4 plane(normal, -glm::dot(normal, point));

        // From behind, the top face is not visible, so there is nothing to
        // reflect. The oblique near plane also degenerates when the camera
        // sits on the plane, so require some clearance in front of it.
        if (glm::dot(glm::vec4(frame.cameraPos, 1.F), plane) > 0.01F) {
            const auto reflection = PlanarReflection::reflectionMatrix(plane);
            const auto view       = frame.cameraView * reflection;
            const auto viewPlane  = glm::transpose(glm::inverse(view)) * plane;
            const auto projection = PlanarReflection::obliqueProjection(
                frame.cameraProjection, viewPlane);
            // Empty when the whole view is past the plane, e.g. a mirror that
            // passes the conservative cull from just outside a corner.
            if (projection) {
                frame.planarViewProj = *projection * view;
                frame.planarViewPos =
                    glm::vec3(reflection * glm::vec4(frame.cameraPos, 1.F));
                frame.planarNormal = normal;
                frame.planarIndex  = *planarObject;
                frame.planarActive = true;

                const sponge::scene::Frustum frustum(frame.cameraProjection *
                                                     view);
                frame.planarObjectVisible.resize(objectWorldBounds.size());
                for (size_t i = 0; i < objectWorldBounds.size(); i++) {
                    frame.planarObjectVisible[i] =
                        frustum.intersects(objectWorldBounds[i]) ? 1 : 0;
                }
            }
        }
    }

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
                glm::normalize(directionalLight.direction), sceneBounds);
            frame.lightSpaceMatrix = shadowMap->getLightSpaceMatrix();

            const sponge::scene::Frustum lightFrustum(frame.lightSpaceMatrix);
            for (size_t i = 0; i < objectMeshWorldBounds.size(); i++) {
                const auto& bounds  = objectMeshWorldBounds[i];
                auto&       visMask = frame.objectMeshVisibleLight[i];
                for (size_t m = 0; m < bounds.size(); m++) {
                    visMask[m] = lightFrustum.intersects(bounds[m]) ? 1 : 0;
                }
            }
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

        frame.ssaoEnabled  = ssaoEnabled;
        frame.ssaoRadius   = ssaoRadius;
        frame.ssrEnabled   = ssrEnabled;
        frame.probeEnabled = probeEnabled && probe != nullptr;
    }

    // Last: everything above is real per-frame state now, not finishLoading()'s
    // pre-seeded defaults.
    frame.populated = true;

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
        RendererAPI::setViewport(0, 0, static_cast<int32_t>(w),
                                 static_cast<int32_t>(h));
        if (fxaa) {
            fxaa->resize(w, h);
        }
        if (taa) {
            taa->resize(w, h);
        }
        if (bloom) {
            bloom->resize(w, h);
        }
        if (ssao) {
            ssao->resize(w, h);
        }
        if (planarReflection) {
            planarReflection->resize(w, h);
        }
        sceneTarget->resize(w, h);
        screenWidth  = static_cast<int32_t>(w);
        screenHeight = static_cast<int32_t>(h);
        depthPrepass->resize(w, h);

        const auto shader = Mesh::getShader();
        shader->setFloat2("screenSize", glm::vec2(static_cast<float>(w),
                                                  static_cast<float>(h)));
        pendingResize.store(false, std::memory_order_relaxed);
    }

    // Read the latest snapshot from the update thread.
    const auto& frame =
        renderFrames[renderReadIndex.load(std::memory_order_acquire)];

    // Pick up whichever occlusion queries resolved since last frame, before
    // any pass decides what's visible.
    if (occlusionCuller) {
        occlusionCuller->pollResults();
    }
    if (shadowOcclusionCuller) {
        shadowOcclusionCuller->pollResults();
    }

    // Decided once, after occlusion results are in: the mirrored render and
    // the composite that draws it must agree, or the composite can draw a
    // stale or cleared reflection texture.
    const bool planarDrawn =
        frame.planarActive && planarReflection &&
        !(occlusionCuller && !occlusionCuller->isVisible(frame.planarIndex));

    // Phase 1: shadow map
    if (frame.shadowEnabled && frame.shadowCastShadow) {
        renderSceneToDepthMap(frame);
    }

    // Phase 1.5: the reflection probe, once, after the shadow map it lights
    // with. Waits for the first fully populated frame: finishLoading()
    // pre-seeds objectModels into both slots before any real
    // captureRenderFrame() runs, so checking objectModels alone would
    // capture from the slot's untouched defaults instead — shadow pass
    // skipped, lightSpaceMatrix never written, numLights 0, ssaoEnabled true.
    if (probe && !probeCaptured && frame.populated) {
        captureProbe(frame);
    }

    // Phase 2: depth prepass
    renderDepthPrepass(frame);

    // Phase 2.5: occlusion queries against the depth just rasterized —
    // results feed next frame's occlusion skip, not this one's.
    renderOcclusionQueries(frame);

    // Phase 2.6: the scene seen in the planar mirror.
    if (planarDrawn) {
        renderPlanarReflection(frame);
    }

    // Phase 3: light culling
    if (clusteredLights && frame.numLights > 0) {
        clusteredLights->update(frame.lightPositions.data(),
                                frame.lightColors.data(),
                                frame.lightAttenuationIndex, frame.numLights,
                                frame.cameraView, frame.cameraProjection);
    }

    // Phase 3.5: SSAO, against the depth prepass's depth + view-space normal.
    // Skipped when disabled: renderGameObjects() also gates the shader's read
    // of the texture on frame.ssaoEnabled, so a stale/unrun texture is never
    // sampled.
    if (frame.ssaoEnabled && ssao) {
        ssao->process(depthPrepass->getDepthTexture(),
                      depthPrepass->getNormalTexture(), frame.cameraProjection,
                      glm::inverse(frame.cameraProjection), frame.ssaoRadius);
    }

    // Phase 4: opaque pass, into the linear HDR scene target.
    const bool fxaaActive = frame.antiAliasing == AntiAliasing::Fxaa && fxaa;
    const bool taaActive  = frame.antiAliasing == AntiAliasing::Taa && taa;

    sceneTarget->begin();

    // Blit prepass depth in so the opaque pass can use GL_LEQUAL (zero
    // overdraw). Both FBOs use GL_DEPTH_COMPONENT24.
    depthPrepass->blitDepthToBound();
    RendererAPI::clearColor();
    RendererAPI::setDepth(DepthFunc::LessEqual, false);

    renderGameObjects(frame);

    // The cubes are in the prepass too, so their depth is already in the
    // buffer — GL_LESS would reject every one of their fragments. They stay
    // on the opaque pass's GL_LEQUAL / depth-write-off state.
    renderLightCubes(frame);

    if (planarDrawn) {
        renderPlanarComposite(frame);
    }

    // Before glass, so glass refracts the reflections. The copy is a
    // different texture from the scene color attachment; the glass pass makes
    // its own copy after this one.
    const auto usesSsr = [&frame](const size_t i) {
        return frame.objectReflectivity[i] > 0.F &&
               !(frame.planarActive && i == frame.planarIndex);
    };
    if (frame.ssrEnabled && ssr &&
        std::ranges::any_of(
            std::views::iota(size_t{ 0 }, frame.objectReflectivity.size()),
            usesSsr)) {
        ssr->apply(sceneTarget->copyColor(), depthPrepass->getDepthTexture(),
                   depthPrepass->getNormalTexture(), frame.cameraProjection,
                   glm::inverse(frame.cameraProjection));
    }

    if (std::ranges::any_of(frame.objectRefraction,
                            &scene::SceneRefraction::refractive)) {
        // The copy is a different texture from the scene color attachment.
        // Sampling that attachment while drawing it is undefined.
        const auto sceneCopy = sceneTarget->copyColor();
        RendererAPI::setAlphaBlend(false);
        RendererAPI::setDepth(DepthFunc::Less, true);
        if (sceneTarget->beginGlass(depthPrepass->getVelocityTexture(),
                                    taaActive)) {
            renderRefractiveObjects(frame, sceneCopy);
            // The shader sampled the prepass depth. The test wrote the
            // renderbuffer. TAA reads the prepass texture, so copy the
            // glass depth back onto it.
            sceneTarget->blitDepthTo(depthPrepass->getFramebuffer(),
                                     frame.screenWidth, frame.screenHeight);
        }
        sceneTarget->endGlass();
        RendererAPI::setAlphaBlend(true);
    }

    RendererAPI::setDepth(DepthFunc::Less, true);
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
        // Simplification: TAA accumulates the tone-mapped image, which is where
        // it already ran. Accumulating in linear with a reversible weighting
        // curve resolves highlight edges better, but that change belongs inside
        // TAA, not in this pass order.
        taa->end();
        taa->apply(depthPrepass->getDepthTexture(),
                   depthPrepass->getVelocityTexture(), frame.invCameraMVP,
                   frame.prevCameraViewProj);
    }

    // Before the first populated frame the scene is the untouched defaults.
    if (capture && frame.populated) {
        recordCapture(fxaaActive, taaActive, bloomTexId);
    }

    RendererAPI::setDepth(DepthFunc::LessEqual, true);
}

void MazeLayer::enableCapture(const uint32_t frames, const bool stats) {
    capture.emplace(frames, stats);
    captureYawStep = 360.F / static_cast<float>(std::max(frames, 1U));
}

void MazeLayer::recordCapture(const bool fxaaActive, const bool taaActive,
                              const uint32_t bloomTexId) {
    if (capture->done()) {
        return;
    }
    capture->beginFrame();

    // Reading a stage costs a GPU stall, so without stats only the last frame
    // is read.
    if (capture->wantsStats() || capture->isLastFrame()) {
        // RGB16F stages, before anything quantizes them to 8 bits.
        capture->record("scene", readTexture(sceneTarget->getTexture()), true);
        if (bloomTexId != 0) {
            capture->record("bloom", readTexture(bloomTexId), true);
        }
        if (fxaaActive) {
            capture->record("tonemap", readTexture(fxaa->getInputTexture()),
                            false);
        } else if (taaActive) {
            capture->record("tonemap", readTexture(taa->getInputTexture()),
                            false);
            capture->record("taa", readTexture(taa->getResolvedTexture()),
                            false);
        }
        // What a screen capture sees: 8 bits, dithered.
        capture->record("backbuffer", readBackBuffer(), false);
    }

    if (capture->done()) {
        capture->report();
        Maze::get().exit();
    }
}

float MazeLayer::getAmbientStrength() const {
    return ambientStrength;
}

void MazeLayer::setAmbientStrength(const float val) {
    ambientStrength = val;

    const auto shader = Mesh::getShader();
    shader->setFloat("ambientStrength", ambientStrength);
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
    shader->setBoolean("directionalLight.castShadow",
                       directionalLight.castShadow);
}

glm::vec3 MazeLayer::getDirectionalLightColor() const {
    return directionalLight.color;
}

void MazeLayer::setDirectionalLightColor(const glm::vec3& color) {
    directionalLight.color = color;

    const auto shader = Mesh::getShader();
    shader->setFloat3("directionalLight.color", directionalLight.color);
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
    shader->setFloat3("directionalLight.direction", directionalLight.direction);
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
    shader->setBoolean("directionalLight.enabled", directionalLight.enabled);
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

        const auto& params = sceneDesc.lighting.point;

        // NOLINTNEXTLINE(bugprone-random-generator-seed) fixed layout
        std::mt19937                   rng(42U);
        std::uniform_real_distribution jitterAngle(-params.jitterAngle,
                                                   params.jitterAngle);
        std::uniform_real_distribution jitterRadius(-params.jitterRadius,
                                                    params.jitterRadius);

        // Lights sit on a fixed ellipse: radiusMin is the short (Z) semi-axis,
        // radiusMin + radiusSpan the long (X) one — the same two knobs that
        // used to bound the spiral now shape the oval.
        const float radiusX = params.radiusMin + params.radiusSpan;
        const float radiusZ = params.radiusMin;

        for (int32_t i = 0; i < numLights; i++) {
            const float angle =
                glm::two_pi<float>() * i / numLights + jitterAngle(rng);
            auto& light    = pointLights.at(i);
            light.color    = params.color;
            light.position = glm::vec3(
                (radiusX + jitterRadius(rng)) * glm::sin(angle), params.height,
                -(radiusZ + jitterRadius(rng)) * glm::cos(angle));
        }
    }

    const auto shader = Mesh::getShader();
    shader->setInteger("numLights", numLights);
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
        shadowMap->bindTexture(1);
    }

    if (ssao) {
        RendererAPI::bindTexture(10, ssao->getTexture());
    }

    const bool useProbe = frame.probeEnabled && probeCaptured;
    shader->setBoolean("probeEnabled", useProbe);
    if (useProbe) {
        RendererAPI::bindTexture(12, probe->getTexture());
    }

    const auto submitStart      = std::chrono::steady_clock::now();
    uint32_t   occlusionVisible = 0;
    for (size_t i = 0; i < frame.objectModels.size(); i++) {
        if (frame.objectRefraction[i].refractive) {
            continue;
        }
        if (occlusionCuller && !occlusionCuller->isVisible(i)) {
            continue;
        }
        occlusionVisible++;

        const auto& modelMatrix = frame.objectModelMatrices[i];

        shader->setMat4("mvp", frame.cameraMVP * modelMatrix);
        shader->setMat4("model", modelMatrix);
        const auto normalMatrix =
            glm::mat4(glm::transpose(glm::inverse(glm::mat3(modelMatrix))));
        shader->setMat4("normalMatrix", normalMatrix);
        shader->setFloat3("emissive", frame.objectEmissives[i]);

        frame.objectModels[i]->render(shader, frame.objectMeshVisible[i]);
    }
    const auto submitUs = std::chrono::duration_cast<std::chrono::microseconds>(
                              std::chrono::steady_clock::now() - submitStart)
                              .count();
    submitMicros.store(static_cast<uint32_t>(submitUs),
                       std::memory_order_relaxed);
    occlusionVisibleCount.store(occlusionVisible, std::memory_order_relaxed);
    occlusionTotalCount.store(static_cast<uint32_t>(frame.objectModels.size()),
                              std::memory_order_relaxed);

    shader->unbind();
}

void MazeLayer::renderRefractiveObjects(const thread::MazeRenderFrame& frame,
                                        const uint32_t sceneCopy) const {
    const auto shader = refractionShader;
    shader->bind();
    if (clusteredLights) {
        clusteredLights->bindSSBOs();
    }

    shader->setMat4("cameraMVP", frame.cameraMVP);
    shader->setFloat3("viewPos", frame.cameraPos);
    shader->setFloat3("viewForward",
                      -glm::vec3(frame.cameraView[0][2], frame.cameraView[1][2],
                                 frame.cameraView[2][2]));
    shader->setFloat2("screenSize",
                      glm::vec2(static_cast<float>(frame.screenWidth),
                                static_cast<float>(frame.screenHeight)));
    shader->setFloat("clusterNear", ClusteredLights::clusterNear);
    shader->setFloat("farPlane", frame.farPlane);
    shader->setInteger("numLights", frame.numLights);
    shader->setInteger("attenuationIndex", frame.lightAttenuationIndex);
    shader->setFloat("evsmBleedThreshold", 0.2F);
    shader->setBoolean("directionalLight.enabled", frame.shadowEnabled);
    shader->setBoolean("directionalLight.castShadow", frame.shadowCastShadow);
    shader->setFloat3("directionalLight.direction", frame.lightDirection);
    shader->setFloat3("directionalLight.color", directionalLight.color);
    shader->setMat4("lightSpaceMatrix", frame.lightSpaceMatrix);

    if (frame.shadowEnabled && frame.shadowCastShadow) {
        shadowMap->bindTexture(1);
    }

    RendererAPI::bindTexture(11, sceneCopy);
    RendererAPI::bindTexture(12, depthPrepass->getDepthTexture());

    const bool useProbe = frame.probeEnabled && probeCaptured;
    shader->setBoolean("probeEnabled", useProbe);
    if (useProbe) {
        const auto& desc = *sceneDesc.probe;
        shader->setFloat3("probePosition", desc.position);
        shader->setFloat3("probeBoxMin", desc.boxMin);
        shader->setFloat3("probeBoxMax", desc.boxMax);
        shader->setFloat("probeMaxMip",
                         static_cast<float>(ReflectionProbe::mipLevels - 1));
        RendererAPI::bindTexture(13, probe->getTexture());
    }

    for (size_t i = 0; i < frame.objectModels.size(); i++) {
        if (!frame.objectRefraction[i].refractive) {
            continue;
        }
        if (occlusionCuller && !occlusionCuller->isVisible(i)) {
            continue;
        }

        const auto& modelMatrix = frame.objectModelMatrices[i];
        const auto& glass       = frame.objectRefraction[i];
        shader->setMat4("mvp", frame.cameraMVP * modelMatrix);
        shader->setMat4("model", modelMatrix);
        shader->setMat4(
            "normalMatrix",
            glm::mat4(glm::transpose(glm::inverse(glm::mat3(modelMatrix)))));
        shader->setMat4("mvpNoJitter", frame.cameraViewProj * modelMatrix);
        shader->setMat4("prevMvpNoJitter",
                        frame.prevCameraViewProj *
                            frame.prevObjectModelMatrices[i]);
        shader->setFloat("ior", glass.ior);
        shader->setFloat("thickness", glass.thickness);
        shader->setFloat3("tint", glass.tint);
        frame.objectModels[i]->render(shader, frame.objectMeshVisible[i]);
    }

    shader->unbind();
}

void MazeLayer::renderDepthPrepass(const thread::MazeRenderFrame& frame) const {
    // Only TAA reads the velocity target, so the other modes mask the writes
    // off. They still carry the plumbing: the RG16F attachment stays
    // allocated and the prepass shader still computes both clip-space
    // varyings. Cheap, but not free — measure before assuming parity with the
    // old depth-only pass.
    const bool writeVelocity = frame.antiAliasing == AntiAliasing::Taa;

    depthPrepass->begin(writeVelocity);

    depthPrepassShader->bind();
    for (size_t i = 0; i < frame.objectModels.size(); ++i) {
        if (frame.objectRefraction[i].refractive) {
            continue;
        }
        if (occlusionCuller && !occlusionCuller->isVisible(i)) {
            continue;
        }

        // Rasterization uses the jittered matrix so prepass depth lines up with
        // the scene pass; motion is measured unjittered, or the jitter itself
        // would read as movement.
        const auto& modelMatrix = frame.objectModelMatrices[i];
        depthPrepassShader->setMat4("mvp", frame.cameraMVP * modelMatrix);
        depthPrepassShader->setMat4(
            "normalMatrix", glm::mat4(glm::transpose(glm::inverse(
                                glm::mat3(frame.cameraView * modelMatrix)))));
        depthPrepassShader->setMat4("mvpNoJitter",
                                    frame.cameraViewProj * modelMatrix);
        depthPrepassShader->setMat4("prevMvpNoJitter",
                                    frame.prevCameraViewProj *
                                        frame.prevObjectModelMatrices[i]);
        // The planar mirror has its own reflection; SSR must skip it.
        const bool planar = frame.planarActive && i == frame.planarIndex;
        depthPrepassShader->setFloat(
            "reflectivity", planar ? 0.F : frame.objectReflectivity[i]);
        frame.objectModels[i]->render(depthPrepassShader,
                                      frame.objectMeshVisible[i]);
    }

    // Light cubes use a second instance of the same shader: position-only
    // geometry at location 0. Including them here is what gives them depth
    // coverage and motion vectors.
    depthPrepassShader->unbind();
    depthPrepassCubeShader->bind();
    const auto cubeScale = glm::vec3(sceneDesc.lighting.point.debugCubeScale);
    depthPrepassCubeShader->setFloat("reflectivity", 0.F);
    for (int32_t i = 0; i < frame.numLights; i++) {
        const auto model = lightCubeModel(frame.lightPositions[i], cubeScale);
        const auto prevModel =
            lightCubeModel(frame.prevLightPositions[i], cubeScale);
        depthPrepassCubeShader->setMat4("mvp", frame.cameraMVP * model);
        depthPrepassCubeShader->setMat4("mvpNoJitter",
                                        frame.cameraViewProj * model);
        depthPrepassCubeShader->setMat4("prevMvpNoJitter",
                                        frame.prevCameraViewProj * prevModel);
        cube->render();
    }

    depthPrepassCubeShader->unbind();

    depthPrepass->end();
}

void MazeLayer::renderOcclusionQueries(
    const thread::MazeRenderFrame& frame) const {
    if (!occlusionCuller) {
        return;
    }

    // Test every object's AABB against the depth prepass just rasterized.
    // The query writes no depth, so it can't perturb the buffer the opaque
    // pass is about to blit and depth-test against.
    depthPrepass->bind();
    occlusionCuller->query(objectWorldBounds, frame.cameraMVP, frame.cameraPos);
    depthPrepass->unbind();
}

void MazeLayer::renderLightCubes(const thread::MazeRenderFrame& frame) const {
    if (frame.numLights == 0) {
        return;
    }

    const auto shader = cube->getShader();
    shader->bind();

    const auto cubeScale = glm::vec3(sceneDesc.lighting.point.debugCubeScale);
    for (int32_t i = 0; i < frame.numLights; i++) {
        shader->setFloat3("lightColor", frame.lightColors[i]);
        shader->setMat4("mvp",
                        frame.cameraMVP *
                            lightCubeModel(frame.lightPositions[i], cubeScale));
        cube->render();
    }

    shader->unbind();
}

void MazeLayer::renderPlanarReflection(
    const thread::MazeRenderFrame& frame) const {
    planarReflection->begin();

    const auto shader = Mesh::getShader();
    shader->bind();
    // The clustered light grid and the SSAO texture are built for the game
    // camera, so the mirrored view uses neither.
    shader->setInteger("numLights", 0);
    shader->setBoolean("ssaoEnabled", false);
    shader->setBoolean("probeEnabled", false);
    shader->setFloat3("viewPos", frame.planarViewPos);
    if (frame.shadowEnabled && frame.shadowCastShadow) {
        shader->setMat4("lightSpaceMatrix", frame.lightSpaceMatrix);
        shadowMap->bindTexture(1);
    }

    for (size_t i = 0; i < frame.objectModels.size(); i++) {
        if (i == frame.planarIndex || frame.objectRefraction[i].refractive ||
            frame.planarObjectVisible[i] == 0) {
            continue;
        }
        const auto& modelMatrix = frame.objectModelMatrices[i];
        shader->setMat4("mvp", frame.planarViewProj * modelMatrix);
        shader->setMat4("model", modelMatrix);
        shader->setMat4(
            "normalMatrix",
            glm::mat4(glm::transpose(glm::inverse(glm::mat3(modelMatrix)))));
        shader->setFloat3("emissive", frame.objectEmissives[i]);
        frame.objectModels[i]->render(shader);
    }

    shader->setInteger("numLights", frame.numLights);
    shader->setBoolean("ssaoEnabled", frame.ssaoEnabled);
    shader->unbind();

    planarReflection->end();
}

void MazeLayer::captureProbe(const thread::MazeRenderFrame& frame) const {
    const auto  start      = std::chrono::steady_clock::now();
    const auto& desc       = *sceneDesc.probe;
    const auto  projection = glm::perspective(glm::radians(90.F), 1.F,
                                              frame.nearPlane, frame.farPlane);

    const auto shader = Mesh::getShader();
    // Like the mirrored render: the light grid and SSAO belong to the game
    // camera. The probe must not sample itself while it is being drawn.
    shader->setInteger("numLights", 0);
    shader->setBoolean("ssaoEnabled", false);
    shader->setBoolean("probeEnabled", false);
    shader->setFloat3("viewPos", desc.position);
    if (frame.shadowEnabled && frame.shadowCastShadow) {
        shader->setMat4("lightSpaceMatrix", frame.lightSpaceMatrix);
        shadowMap->bindTexture(1);
    }

    for (int face = 0; face < 6; face++) {
        // beginFace() clears the face. On NVIDIA, a clear with this program
        // bound recompiles its vertex shader, so bind after the clear.
        probe->beginFace(face);
        shader->bind();
        const auto viewProj =
            projection * ReflectionProbe::faceView(desc.position, face);
        for (size_t i = 0; i < frame.objectModels.size(); i++) {
            if (frame.objectRefraction[i].refractive) {
                continue;
            }
            const auto& modelMatrix = frame.objectModelMatrices[i];
            shader->setMat4("mvp", viewProj * modelMatrix);
            shader->setMat4("model", modelMatrix);
            shader->setMat4("normalMatrix",
                            glm::mat4(glm::transpose(
                                glm::inverse(glm::mat3(modelMatrix)))));
            shader->setFloat3("emissive", frame.objectEmissives[i]);
            frame.objectModels[i]->render(shader);
        }
        shader->unbind();
        probe->end();
    }

    shader->setInteger("numLights", frame.numLights);
    shader->setBoolean("ssaoEnabled", frame.ssaoEnabled);

    probe->prefilter();

    shader->setFloat3("probePosition", desc.position);
    shader->setFloat3("probeBoxMin", desc.boxMin);
    shader->setFloat3("probeBoxMax", desc.boxMax);
    shader->setFloat("probeMaxMip",
                     static_cast<float>(ReflectionProbe::mipLevels - 1));

    probeCaptured = true;

    // Unused when the log level compiles SPONGE_INFO out (release).
    [[maybe_unused]] const auto ms =
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start)
            .count();
    SPONGE_INFO("Reflection probe captured in {:.1f} ms", ms);
}

void MazeLayer::renderPlanarComposite(
    const thread::MazeRenderFrame& frame) const {
    const auto i = frame.planarIndex;
    // The global default, set here like Ssr does, so the blend does not
    // depend on the passes before it.
    RendererAPI::setAlphaBlend(true);

    const auto& modelMatrix = frame.objectModelMatrices[i];
    planarShader->bind();
    planarShader->setMat4("mvp", frame.cameraMVP * modelMatrix);
    planarShader->setMat4(
        "normalMatrix",
        glm::mat4(glm::transpose(glm::inverse(glm::mat3(modelMatrix)))));
    planarShader->setFloat3("planeNormal", frame.planarNormal);
    // The render thread's size, not the frame's: on a resize frame the
    // reflection image already has the new size.
    planarShader->setFloat2("screenSize",
                            glm::vec2(static_cast<float>(screenWidth.load()),
                                      static_cast<float>(screenHeight.load())));
    planarShader->setFloat("strength", frame.objectReflectivity[i]);
    RendererAPI::bindTexture(11, planarReflection->getTexture());
    frame.objectModels[i]->render(planarShader, frame.objectMeshVisible[i]);
    planarShader->unbind();
}

void MazeLayer::renderSceneToDepthMap(
    const thread::MazeRenderFrame& frame) const {
    shadowMap->bind();

    const auto shader = shadowMap->getShader();
    shader->bind();
    shader->setMat4("lightSpaceMatrix", frame.lightSpaceMatrix);

    for (size_t i = 0; i < frame.objectModels.size(); i++) {
        if (frame.objectRefraction[i].refractive) {
            continue;
        }
        if (shadowOcclusionCuller && !shadowOcclusionCuller->isVisible(i)) {
            continue;
        }
        shader->setMat4("model", frame.objectModelMatrices[i]);
        frame.objectModels[i]->render(shader, frame.objectMeshVisibleLight[i]);
    }

    shader->unbind();

    // Test every object's AABB against the shadow depth just rasterized,
    // same technique as renderOcclusionQueries() but against the light's own
    // depth instead of the camera's — still bound, so no FBO switch needed.
    if (shadowOcclusionCuller) {
        shadowOcclusionCuller->query(objectWorldBounds, frame.lightSpaceMatrix,
                                     shadowMap->getEyePosition());
    }

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

    if (captureYawStep != 0.F) {
        camera->mouseMove({ captureYawStep, 0.F });
    }

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

bool MazeLayer::isSsaoEnabled() const {
    return ssaoEnabled;
}

void MazeLayer::setSsaoEnabled(const bool val) {
    {
        std::scoped_lock lock(settingsMutex);
        ssaoEnabled = val;
    }

    const auto shader = Mesh::getShader();
    shader->setBoolean("ssaoEnabled", val);
}

float MazeLayer::getSsaoRadius() const {
    return ssaoRadius;
}

void MazeLayer::setSsaoRadius(const float val) {
    std::scoped_lock lock(settingsMutex);
    ssaoRadius = val;
}

bool MazeLayer::isSsrEnabled() const {
    return ssrEnabled;
}

void MazeLayer::setSsrEnabled(const bool val) {
    std::scoped_lock lock(settingsMutex);
    ssrEnabled = val;
}

bool MazeLayer::isPlanarEnabled() const {
    return planarEnabled;
}

void MazeLayer::setPlanarEnabled(const bool val) {
    std::scoped_lock lock(settingsMutex);
    planarEnabled = val;
}

bool MazeLayer::isProbeEnabled() const {
    return probeEnabled;
}

void MazeLayer::setProbeEnabled(const bool val) {
    std::scoped_lock lock(settingsMutex);
    probeEnabled = val;
}

bool MazeLayer::isImguiActive() const {
    return isImguiOpen;
}
}  // namespace game::layer
