#pragma once

#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "event/mouseevent.hpp"
#include "input/inputsnapshot.hpp"
#include "layer/layer.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/scene/bloom.hpp"
#include "platform/opengl/scene/clusteredlights.hpp"
#include "platform/opengl/scene/cube.hpp"
#include "platform/opengl/scene/fxaa.hpp"
#include "platform/opengl/scene/model.hpp"
#include "platform/opengl/scene/shadowmap.hpp"
#include "platform/opengl/scene/taa.hpp"
#include "scene/gamecamera.hpp"
#include "thread/mazeframe.hpp"

#include <glm/glm.hpp>

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

namespace game::layer {
struct GameObject {
    std::string_view name;
    std::string_view path;
    glm::vec3        scale{ 1.F };
    struct {
        float     angle{ 0.F };
        glm::vec3 axis{ 0.F, 1.F, 0.F };
    } rotation{};
    glm::vec3 translation{ 0.F };
    glm::vec3 emissive{ 0.F };
};

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

    float getAmbientOcclusion() const;

    void setAmbientOcclusion(float val);

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

    int32_t getNumLights() const;

    void setNumLights(int32_t val);

    thread::AntiAliasing getAntiAliasing() const;

    void setAntiAliasing(thread::AntiAliasing val);

    bool  isBloomEnabled() const;
    void  setBloomEnabled(bool val);
    float getBloomThreshold() const;
    void  setBloomThreshold(float val);
    float getBloomIntensity() const;
    void  setBloomIntensity(float val);

#ifdef ENABLE_IMGUI
    bool isImguiActive() const;
#endif

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

private:
    std::shared_ptr<scene::GameCamera> camera;
    std::vector<glm::mat4>             objectModelMatrices;
    std::vector<glm::vec3>             objectEmissives;
    std::vector<std::shared_ptr<sponge::platform::opengl::scene::Model>>
        objectModels;
    std::unique_ptr<sponge::platform::opengl::scene::ClusteredLights>
        clusteredLights;
    std::shared_ptr<sponge::platform::opengl::renderer::Shader>
             depthPrepassShader;
    uint32_t depthPrepassFbo{ 0 };
    uint32_t depthPrepassTexture{ 0 };
    // Screen-space motion (RG16F, current UV minus previous UV) written by the
    // depth prepass and consumed by TAA. Shares the prepass FBO.
    uint32_t                                               velocityTexture{ 0 };
    std::unique_ptr<sponge::platform::opengl::scene::Cube> cube;
    std::unique_ptr<sponge::platform::opengl::scene::FXAA> fxaa;
    std::unique_ptr<sponge::platform::opengl::scene::TAA>  taa;
    std::unique_ptr<sponge::platform::opengl::scene::Bloom>     bloom;
    std::unique_ptr<sponge::platform::opengl::scene::ShadowMap> shadowMap;

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
    float                ambientStrength  = .25F;
    float                ao               = .25F;
    int32_t              attenuationIndex = 4;
    thread::AntiAliasing antiAliasing     = thread::AntiAliasing::Taa;
    bool                 bloomEnabled     = true;
    float                bloomThreshold   = 0.8F;
    // Compensates the soft-knee extract, which passes only above-threshold
    // energy (the old hard threshold passed the full pixel color).
    float   bloomIntensity     = 2.5F;
    bool    mouseButtonPressed = false;
    int32_t numLights          = 0;
#ifdef ENABLE_IMGUI
    bool isImguiOpen = true;
#endif

    void onWindowFocus(const sponge::event::WindowFocusEvent& event);

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseButtonReleased(
        const sponge::event::MouseButtonReleasedEvent& event);

    bool onMouseScrolled(const sponge::event::MouseScrolledEvent& event) const;

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;

    void createDepthPrepassFbo(int w, int h);

    void renderDepthPrepass(const thread::MazeRenderFrame& frame) const;

    void blitDepthToCurrentFbo(int w, int h) const;

    void renderGameObjects(const thread::MazeRenderFrame& frame) const;

    void renderLightCubes(const thread::MazeRenderFrame& frame) const;

    void renderSceneToDepthMap(const thread::MazeRenderFrame& frame) const;

    void updateCamera(const sponge::input::InputSnapshot& snap,
                      double                              elapsedTime) const;
};
}  // namespace game::layer
