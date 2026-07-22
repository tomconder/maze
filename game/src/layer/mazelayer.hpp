#pragma once

#include "input/inputsnapshot.hpp"
#include "platform/opengl/scene/clusteredlights.hpp"
#include "scene/gamecamera.hpp"
#include "sponge.hpp"
#include "thread/mazeframe.hpp"

#include <array>
#include <atomic>
#include <memory>
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

    void onAttach() override;

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    // Update thread: camera logic + fill per-frame snapshot. No GL calls.
    bool onUpdate(double elapsedTime) override;

    // Render thread: all GL commands, reads from latest update snapshot.
    void onRender() override;

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

    bool isFxaaEnabled() const;

    void setFxaaEnabled(bool val);

    bool  isBloomEnabled() const;
    void  setBloomEnabled(bool val);
    float getBloomThreshold() const;
    void  setBloomThreshold(float val);
    float getBloomIntensity() const;
    void  setBloomIntensity(float val);

#ifdef ENABLE_IMGUI
    bool isImguiActive() const;
#endif

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
    std::unique_ptr<sponge::platform::opengl::scene::Cube>      cube;
    std::unique_ptr<sponge::platform::opengl::scene::FXAA>      fxaa;
    std::unique_ptr<sponge::platform::opengl::scene::Bloom>     bloom;
    std::unique_ptr<sponge::platform::opengl::scene::ShadowMap> shadowMap;

    // Double-buffered snapshots: update writes, render reads, no overlap.
    std::array<thread::MazeRenderFrame, 2> renderFrames;
    std::atomic<uint32_t>                  renderReadIndex{ 0 };

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
    void queueResize(uint32_t w, uint32_t h) const;

    std::atomic<int32_t> screenWidth{ 0 };
    std::atomic<int32_t> screenHeight{ 0 };
    float                ambientStrength  = .25F;
    float                ao               = .25F;
    int32_t              attenuationIndex = 4;
    bool                 fxaaEnabled      = true;
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
