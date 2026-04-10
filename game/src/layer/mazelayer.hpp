#pragma once

#include "scene/gamecamera.hpp"
#include "sponge.hpp"
#include "thread/mazeframe.hpp"

#include <array>
#include <atomic>
#include <memory>
#include <string_view>

namespace game::layer {
struct GameObject {
    std::string_view name;
    std::string_view path;
    glm::vec3        scale{ 1.F };
    struct {
        float     angle{ 0.F };
        glm::vec3 axis{ 0.F, 1.F, 0.F };
    } rotation;
    glm::vec3 translation{ 0.F };
    glm::mat4 modelMatrix{ 1.F };
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

    float getDirectionalLightShadowBias() const;

    void setDirectionalLightShadowBias(float value);

    uint32_t getDirectionalLightShadowMapRes() const;

    bool isMetallic() const;

    void setMetallic(bool val);

    int32_t getNumLights() const;

    void setNumLights(int32_t val);

    float getRoughness() const;

    void setRoughness(float val);

    float getShadowMapOrthoSize() const;

    void setShadowMapOrthoSize(float val) const;

    uint32_t getShadowMapTextureId() const;

    float getShadowMapZFar() const;

    void setShadowMapZFar(float val) const;

    float getShadowMapZNear() const;

    void setShadowMapZNear(float val) const;

    bool isFxaaEnabled() const;

    void setFxaaEnabled(bool val);

#ifdef ENABLE_IMGUI
    bool isImguiActive() const;
#endif

private:
    std::shared_ptr<scene::GameCamera>                          camera;
    std::unique_ptr<sponge::platform::opengl::scene::Cube>      cube;
    std::unique_ptr<sponge::platform::opengl::scene::FXAA>      fxaa;
    std::unique_ptr<sponge::platform::opengl::scene::ShadowMap> shadowMap;
    std::unordered_map<sponge::input::KeyCode, bool>            keyPressed;

    // Double-buffered snapshots: update writes, render reads, no overlap.
    std::array<thread::MazeRenderFrame, 2> renderFrames;
    std::atomic<uint32_t>                  renderReadIndex{ 0 };

    // Deferred FXAA resize: set by onWindowResize(), applied in onRender().
    mutable std::atomic<bool>     pendingResize{ false };
    mutable std::atomic<uint32_t> pendingResizeWidth{ 0 };
    mutable std::atomic<uint32_t> pendingResizeHeight{ 0 };

    void captureRenderFrame(uint32_t slotIndex);

    float   ambientStrength    = .25F;
    float   ao                 = .25F;
    int32_t attenuationIndex   = 4;
    bool    fxaaEnabled        = true;
    bool    metallic           = false;
    bool    mouseButtonPressed = false;
    int32_t numLights          = 0;
    float   roughness          = .5F;
#ifdef ENABLE_IMGUI
    bool isImguiOpen = true;
#endif

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event);

    void onWindowFocus(const sponge::event::WindowFocusEvent& event);

    bool onKeyReleased(const sponge::event::KeyReleasedEvent& event);

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseButtonReleased(
        const sponge::event::MouseButtonReleasedEvent& event);

    bool onMouseMoveEvent(const sponge::event::MouseMovedEvent& event);

    bool onMouseScrolled(const sponge::event::MouseScrolledEvent& event) const;

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;

    void renderGameObjects(const thread::MazeRenderFrame& frame) const;

    void renderLightCubes(const thread::MazeRenderFrame& frame) const;

    void renderSceneToDepthMap(const thread::MazeRenderFrame& frame) const;

    void updateCamera(double elapsedTime) const;

    void updateShaderLights() const;

    bool isKeyPressed(sponge::input::KeyCode key) const;
};
}  // namespace game::layer
