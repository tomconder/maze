#pragma once

#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "event/mouseevent.hpp"
#include "input/inputsnapshot.hpp"
#include "layer/framecapture.hpp"
#include "layer/layer.hpp"
#include "platform/opengl/debug/gputimer.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/scene/bloom.hpp"
#include "platform/opengl/scene/clusteredlights.hpp"
#include "platform/opengl/scene/cube.hpp"
#include "platform/opengl/scene/depthprepass.hpp"
#include "platform/opengl/scene/fxaa.hpp"
#include "platform/opengl/scene/model.hpp"
#include "platform/opengl/scene/occlusionculler.hpp"
#include "platform/opengl/scene/planarreflection.hpp"
#include "platform/opengl/scene/reflectionprobe.hpp"
#include "platform/opengl/scene/scenetarget.hpp"
#include "platform/opengl/scene/shadowmap.hpp"
#include "platform/opengl/scene/ssao.hpp"
#include "platform/opengl/scene/ssr.hpp"
#include "platform/opengl/scene/taa.hpp"
#include "scene/frustum.hpp"
#include "scene/gamecamera.hpp"
#include "scene/scenefile.hpp"
#include "thread/mazeframe.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace game::layer {
class MazeLayer final : public sponge::layer::Layer {
public:
    MazeLayer();

    // onUpdate() runs on the update thread (no GL); onRender() issues all GPU
    // commands.
    bool runsOnUpdateThread() const override {
        return true;
    }

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    // Update thread: camera logic + fill per-frame snapshot. No GL calls.
    bool onUpdate(double elapsedTime) override;

    // Render thread: all GL commands, reads from latest update snapshot.
    void onRender() override;

    // Main thread, both workers idle: publish the slot written by the last
    // completed onUpdate() so render[N] always reads update[N-1]'s frame.
    void onFrameSync() override;

    float getAmbientStrength() const;

    void setAmbientStrength(float val);

    int32_t getAttenuationIndex() const;

    void setAttenuationIndex(int32_t val);

    std::shared_ptr<scene::GameCamera> getCamera() const;

    bool getDirectionalLightCastsShadow() const;

    void setDirectionalLightCastsShadow(bool value);

    glm::vec3 getDirectionalLightColor() const;

    void setDirectionalLightColor(const glm::vec3& color);

    glm::vec3 getDirectionalLightDirection() const;

    void setDirectionalLightDirection(const glm::vec3& direction);

    bool getDirectionalLightEnabled() const;

    void setDirectionalLightEnabled(bool value);

    uint32_t getDirectionalLightShadowMapRes() const;

    void setShadowMapRes(uint32_t res);

    uint32_t getShadowCascades() const {
        return shadowCascades.load(std::memory_order_relaxed);
    }
    void setShadowCascades(const uint32_t count) {
        shadowCascades.store(
            std::clamp(count, 1U,
                       sponge::platform::opengl::scene::ShadowMap::maxCascades),
            std::memory_order_relaxed);
    }
    float getShadowSplitLambda() const {
        return shadowSplitLambda.load(std::memory_order_relaxed);
    }
    void setShadowSplitLambda(const float lambda) {
        shadowSplitLambda.store(std::clamp(lambda, 0.F, 1.F),
                                std::memory_order_relaxed);
    }

    int32_t getNumLights() const;

    void setNumLights(int32_t val);

    thread::AntiAliasing getAntiAliasing() const;

    void setAntiAliasing(thread::AntiAliasing val);

    bool                                        isBloomEnabled() const;
    void                                        setBloomEnabled(bool val);
    float                                       getBloomThreshold() const;
    void                                        setBloomThreshold(float val);
    float                                       getBloomIntensity() const;
    void                                        setBloomIntensity(float val);
    sponge::platform::opengl::scene::ToneMapper getToneMapper() const;
    void setToneMapper(sponge::platform::opengl::scene::ToneMapper val);

    bool  isSsaoEnabled() const;
    void  setSsaoEnabled(bool val);
    float getSsaoRadius() const;
    void  setSsaoRadius(float val);

    bool isSsrEnabled() const;
    void setSsrEnabled(bool val);

    bool isPlanarEnabled() const;
    void setPlanarEnabled(bool val);

    bool isProbeEnabled() const;
    void setProbeEnabled(bool val);

    bool isImguiActive() const;

    // This frame's frustum-culling stats, for the debug UI. Updated on the
    // update thread in captureRenderFrame(), read from the render thread.
    uint32_t getVisibleMeshCount() const {
        return visibleMeshCount.load(std::memory_order_relaxed);
    }
    uint32_t getTotalMeshCount() const {
        return totalMeshCount.load(std::memory_order_relaxed);
    }

    // Wall-clock cost of the cull test itself (update thread) and of the
    // per-object CPU submission loop it feeds (render thread) — what the
    // culling spends vs. what it's saving on the meshes it drops.
    uint32_t getCullMicros() const {
        return cullMicros.load(std::memory_order_relaxed);
    }
    uint32_t getSubmitMicros() const {
        return submitMicros.load(std::memory_order_relaxed);
    }

    // This frame's occlusion-culling stats, for the debug UI. Written from
    // renderGameObjects() (render thread), read from the render thread too —
    // atomic only so the type matches the frustum counters above.
    uint32_t getOcclusionVisibleCount() const {
        return occlusionVisibleCount.load(std::memory_order_relaxed);
    }
    uint32_t getOcclusionTotalCount() const {
        return occlusionTotalCount.load(std::memory_order_relaxed);
    }

    // True once finishLoading() has run; LoadingLayer skips reloading if set.
    bool isLoaded() const {
        return resourcesReady.load(std::memory_order_acquire);
    }

    // Load requests for LoadingLayer, in the order finishLoading() expects.
    std::vector<sponge::platform::opengl::scene::ModelCreateInfo>
        getModelLoadRequests() const;

    // Finishes sync setup (camera/shader/shadow map/FXAA/bloom/clustered
    // lights) from LoadingLayer's built models and activates the layer.
    void finishLoading(
        std::vector<std::shared_ptr<sponge::platform::opengl::scene::Model>>
            builtModels);

    // Re-activates an already-loaded layer without going through LoadingLayer.
    void activate();

    // Debug capture run: renders `frames` frames while the camera turns once,
    // then the app exits. See FrameCapture. Call before the app runs.
    // `off` is a comma list of passes to disable for the run, to find which
    // one costs frame time: aa, bloom, ssao, ssr, shadow, probe, planar. Two
    // more change how the run measures: shadowcache draws the shadow map every
    // frame, and gputimer turns the per-pass GPU timer off. A non-zero `walk`
    // moves the camera that many world units per update along +X and keeps its
    // yaw fixed, instead of turning it: a walk shows how often a cascade
    // redraws, which a turn cannot.
    // A non-zero `cascades` sets the shadow cascade count for the run.
    void enableCapture(uint32_t frames, bool stats, std::string off = {},
                       float walk = 0.F, uint32_t cascades = 0);

private:
    // Loaded in the constructor, before any other member reads it.
    const scene::Scene sceneDesc;

    std::shared_ptr<scene::GameCamera> camera;
    std::vector<glm::mat4>             objectModelMatrices;
    std::vector<glm::vec3>             objectEmissives;
    std::vector<float>                 objectReflectivity;
    // Index of the one object drawn with planar reflection, if any.
    std::optional<size_t>               planarObject;
    std::vector<scene::SceneRefraction> objectRefraction;
    std::vector<std::shared_ptr<sponge::platform::opengl::scene::Model>>
        objectModels;
    // World-space per-mesh AABB, index-locked with objectModels/[mesh index].
    // Objects never move after finishLoading(), so this is computed once
    // there rather than every frame like the visibility test that reads it.
    std::vector<std::vector<sponge::scene::AABB>> objectMeshWorldBounds;
    // Per-object union of objectMeshWorldBounds, index-locked with
    // objectModels — the box occlusionCuller tests each object against. Same
    // one-time computation, same reason: objects never move.
    std::vector<sponge::scene::AABB> objectWorldBounds;
    // Union of objectMeshWorldBounds, for fitting the shadow frustum to the
    // scene. Same one-time computation as above, same reason.
    sponge::scene::AABB sceneBounds;
    // Hardware occlusion queries against the depth prepass, gating the
    // camera-view passes (depth prepass, opaque). One frame of latency; see
    // occlusionculler.hpp.
    std::unique_ptr<sponge::platform::opengl::scene::OcclusionCuller>
        occlusionCuller;
    std::unique_ptr<sponge::platform::opengl::scene::ClusteredLights>
        clusteredLights;
    std::shared_ptr<sponge::platform::opengl::renderer::Shader>
        depthPrepassShader;
    std::shared_ptr<sponge::platform::opengl::renderer::Shader>
        depthPrepassCubeShader;
    std::shared_ptr<sponge::platform::opengl::renderer::Shader>
        refractionShader;
    // The scene seen in the planar mirror, and the shader that draws it on
    // the mirror.
    std::unique_ptr<sponge::platform::opengl::scene::PlanarReflection>
        planarReflection;
    std::shared_ptr<sponge::platform::opengl::renderer::Shader> planarShader;
    // The scene captured once from the scene file's probe position; empty
    // when the scene has no probe.
    std::unique_ptr<sponge::platform::opengl::scene::ReflectionProbe> probe;
    // Render thread only: set after the first capture.
    mutable bool probeCaptured{ false };
    std::unique_ptr<sponge::platform::opengl::scene::DepthPrepass> depthPrepass;
    std::unique_ptr<sponge::platform::opengl::scene::Cube>         cube;
    std::unique_ptr<sponge::platform::opengl::scene::FXAA>         fxaa;
    std::unique_ptr<sponge::platform::opengl::scene::TAA>          taa;
    std::unique_ptr<sponge::platform::opengl::scene::Bloom>        bloom;
    std::unique_ptr<sponge::platform::opengl::scene::Ssao>         ssao;
    std::unique_ptr<sponge::platform::opengl::scene::Ssr>          ssr;
    std::unique_ptr<sponge::platform::opengl::scene::SceneTarget>  sceneTarget;
    std::unique_ptr<sponge::platform::opengl::scene::ShadowMap>    shadowMap;

    // Cascades in use, 1 to ShadowMap::maxCascades, and the log/uniform blend
    // of their split distances. Read on the update thread; ImGui and the debug
    // capture set them from the render thread.
    std::atomic<uint32_t> shadowCascades{ 2 };
    std::atomic<float>    shadowSplitLambda{ 0.7F };

    // Render thread only. A cascade is drawn again only when its matrix
    // changes or the map is rebuilt. This assumes the shadow casters never
    // move, which holds while objectModelMatrices is static after load;
    // animated casters must invalidate it too.
    sponge::platform::opengl::scene::ShadowMap::Matrices shadowCachedMatrices{};
    std::array<bool, sponge::platform::opengl::scene::ShadowMap::maxCascades>
        shadowCached{};
    // Debug capture only: draw the shadow map every frame, to time it.
    bool shadowCacheOff = false;
    // Debug capture only: skip the GPU timer, to measure its own cost.
    bool gpuTimerOff = false;

    // Double-buffered snapshots: update writes, render reads, no overlap.
    std::array<thread::MazeRenderFrame, 2> renderFrames;
    std::atomic<uint32_t>                  renderReadIndex{ 0 };

    // Slot filled by the last completed captureRenderFrame(). Written on the
    // update thread, read in onFrameSync() on the main thread — ordered by
    // the Worker wait/kick handshake, so no atomic needed.
    uint32_t writtenSlot{ 0 };

    // Deferred viewport/FXAA resize: set by onWindowResize(), applied in
    // onRender(). Dimensions are packed into one uint64_t (width << 32 |
    // height) so the pair is always read and written atomically — no torn
    // width/height.
    mutable std::atomic<bool>     pendingResize{ false };
    mutable std::atomic<uint64_t> pendingResizeDimensions{ 0 };

    // Deferred shadow map FBO rebuild: set from any thread, applied in
    // onRender() on the GL thread.
    mutable std::atomic<bool>     pendingShadowRebuild{ false };
    mutable std::atomic<uint32_t> pendingShadowRebuildRes{ 0 };

    void captureRenderFrame(uint32_t slotIndex);

    // Update-thread only: the previous snapshot's unjittered view-projection,
    // published into the next frame so TAA's camera history and object history
    // always describe the same past frame. Never read the live camera at
    // resolve time — render[N] reads update[N-1], so it is frames ahead.
    glm::mat4 prevCameraViewProj{ 1.F };

    // Update-thread only: last frame's light positions, published into the
    // next snapshot so the light cubes get motion vectors of their own.
    std::array<glm::vec3, thread::MazeRenderFrame::maxLights>
        prevLightPositions{};

    // Update-thread only: index into the Halton jitter sequence.
    uint32_t jitterIndex{ 0 };

    void queueResize(uint32_t w, uint32_t h) const;

    // Set by finishLoading(); onUpdate()/onRender() no-op until then.
    std::atomic<bool> resourcesReady{ false };

    // Guards settings written by ImGui (render thread) and read by
    // captureRenderFrame() (update thread): lights, directional light,
    // fxaa/bloom params. Uncontended except while a debug slider is dragged.
    mutable std::mutex settingsMutex;

    std::atomic<int32_t> screenWidth{ 0 };
    std::atomic<int32_t> screenHeight{ 0 };

    // Frustum-cull stats from the last captureRenderFrame(), for the debug
    // UI. Written on the update thread, read on the render thread.
    std::atomic<uint32_t> visibleMeshCount{ 0 };
    std::atomic<uint32_t> totalMeshCount{ 0 };
    std::atomic<uint32_t> cullMicros{ 0 };
    // Set from renderGameObjects(), which is const (render-thread methods
    // are const throughout this class).
    mutable std::atomic<uint32_t> submitMicros{ 0 };
    mutable std::atomic<uint32_t> occlusionVisibleCount{ 0 };
    mutable std::atomic<uint32_t> occlusionTotalCount{ 0 };
    float                         ambientStrength  = .25F;
    float                         ao               = .25F;
    int32_t                       attenuationIndex = 4;
    thread::AntiAliasing          antiAliasing     = thread::AntiAliasing::Taa;
    sponge::platform::opengl::scene::ToneMapper toneMapper =
        sponge::platform::opengl::scene::ToneMapper::Agx;
    bool  bloomEnabled   = true;
    float bloomThreshold = 0.8F;
    // Additive weight for the bloom texture, applied in LINEAR space before
    // tone mapping. The bloom texture holds radiance (measured peak ~2.6 in
    // this scene), not the [0,1] tone-mapped values it held when bloom
    // composited after the curve, so this is roughly 1/30th of the old value
    // for the same look. Re-derive it against a measurement, never by
    // scaling the old number.
    float   bloomIntensity     = 0.08F;
    bool    ssaoEnabled        = true;
    float   ssaoRadius         = 0.5F;
    bool    ssrEnabled         = true;
    bool    planarEnabled      = true;
    bool    probeEnabled       = true;
    bool    mouseButtonPressed = false;
    int32_t numLights          = 0;
    bool    isImguiOpen        = true;

    std::optional<FrameCapture> capture;
    std::string                 captureOff;
    // Capture runs only; null otherwise, which turns gpuBegin and gpuEnd into
    // no-ops.
    mutable std::optional<sponge::platform::opengl::debug::GpuTimer> gpuTimer;
    // Degrees of yaw per update while capturing; 0 leaves the camera alone.
    float captureYawStep = 0.F;
    // World units per update along +X while capturing; 0 leaves the camera
    // alone.
    float captureWalkStep = 0.F;

    void onWindowFocus(const sponge::event::WindowFocusEvent& event);

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseButtonReleased(
        const sponge::event::MouseButtonReleasedEvent& event);

    bool onMouseScrolled(const sponge::event::MouseScrolledEvent& event) const;

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;

    void renderDepthPrepass(const thread::MazeRenderFrame& frame) const;

    void renderOcclusionQueries(const thread::MazeRenderFrame& frame) const;

    void renderGameObjects(const thread::MazeRenderFrame& frame) const;

    void renderRefractiveObjects(const thread::MazeRenderFrame& frame,
                                 uint32_t sceneCopy) const;

    void renderLightCubes(const thread::MazeRenderFrame& frame) const;
    void recordCapture(bool fxaaActive, bool taaActive, uint32_t bloomTexId);
    void gpuBegin(std::string_view name) const {
        if (gpuTimer) {
            gpuTimer->begin(name);
        }
    }
    void gpuEnd() const {
        if (gpuTimer) {
            gpuTimer->end();
        }
    }

    void renderPlanarReflection(const thread::MazeRenderFrame& frame) const;

    void captureProbe(const thread::MazeRenderFrame& frame) const;

    void renderPlanarComposite(const thread::MazeRenderFrame& frame) const;

    // Draws one cascade into the shadow map, then blurs it.
    void renderSceneToDepthMap(const thread::MazeRenderFrame& frame,
                               uint32_t                       cascade) const;

    // Sets the cascade matrices and count on a lit shader and binds the
    // shadow array on unit 1.
    void bindShadow(const sponge::platform::opengl::renderer::Shader& shader,
                    const thread::MazeRenderFrame& frame) const;

    void updateCamera(const sponge::input::InputSnapshot& snap,
                      double                              elapsedTime) const;
};
}  // namespace game::layer
