#pragma once

#include "platform/opengl/scene/clusteredlights.hpp"
#include "platform/opengl/scene/model.hpp"
#include "platform/opengl/scene/scenetarget.hpp"
#include "platform/opengl/scene/shadowmap.hpp"
#include "scene/clustercut.hpp"
#include "scene/refraction.hpp"
#include "thread/livesettings.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace game::thread {

// Immutable snapshot of all rendering-relevant game state for one frame.
//
// The update thread fills this struct at the end of onUpdate() via
// captureRenderFrame(). The render thread reads from it in onRender().
// Two slots allow the update thread to write one slot while the render
// thread is reading the other — no locks needed because the main-loop
// synchronization ensures the writer and reader are always in different
// slots at any given moment.
struct MazeRenderFrame {
    // True once captureRenderFrame() has filled this slot with real
    // per-frame state at least once. finishLoading() pre-seeds objectModels
    // (and the other per-object arrays) into both slots before any capture
    // runs, so a non-empty objectModels alone does not mean this slot holds
    // a real frame — check this instead.
    bool populated{ false };

    // Camera
    // Jittered when TAA is on — this is what the geometry passes use.
    glm::mat4 cameraMVP{ 1.F };
    // Unjittered view-projection, and the inverse of the jittered one: TAA
    // reprojects through the depth buffer between the two.
    glm::mat4 cameraViewProj{ 1.F };
    // The previous snapshot's unjittered view-projection. Carried here rather
    // than kept on the render thread so it always pairs with
    // prevObjectModelMatrices below — both describe the same past frame.
    glm::mat4 prevCameraViewProj{ 1.F };
    glm::mat4 invCameraMVP{ 1.F };
    glm::mat4 cameraView{ 1.F };
    glm::mat4 cameraProjection{ 1.F };
    glm::vec3 cameraPos{ 0.F };
    float     nearPlane{ 0.1F };
    float     farPlane{ 100.F };
    int       screenWidth{ 0 };
    int       screenHeight{ 0 };

    // The settings as they stood when this frame was captured. One copy for
    // the whole frame, so no pass sees a mix of old and new values. The
    // directional light, point light count, post-processing and reflection
    // flags are read from here.
    LiveSettings settings;

    // Directional light / shadow
    // One matrix per cascade in use; the last one covers the whole scene.
    std::array<glm::mat4,
               sponge::platform::opengl::scene::ShadowMap::maxCascades>
             lightSpaceMatrices{};
    uint32_t cascadeCount{ 1 };

    static constexpr size_t maxLights = static_cast<size_t>(
        sponge::platform::opengl::scene::ClusteredLights::maxLights);

    // Point lights (one attenuation index shared by all)
    std::array<glm::vec3, maxLights> lightPositions{};
    std::array<glm::vec3, maxLights> prevLightPositions{};
    std::array<glm::vec3, maxLights> lightColors{};

    // Game objects: model matrices and resolved model handles. Handles are
    // resolved once at load; no per-frame name lookup.
    std::vector<glm::mat4> objectModelMatrices;
    std::vector<glm::mat4> prevObjectModelMatrices;
    std::vector<glm::vec3> objectEmissives;

    // Static after load. Index-locked with objectModels. 0 means no
    // reflection; the depth prepass writes it into the normal alpha.
    std::vector<float> objectReflectivity;

    // Index-locked with objectModels. A non-refractive object stays in the
    // opaque passes. Filled from the scene each frame, and left blank (all
    // non-refractive) while refraction is off, so glass takes the opaque path.
    std::vector<scene::SceneRefraction> objectRefraction;
    bool                                refractionEnabled{ false };
    std::vector<std::shared_ptr<sponge::platform::opengl::scene::Model>>
        objectModels;

    // Per-mesh camera-frustum visibility, index-locked with objectModels:
    // objectMeshVisible[i][m] is mesh m of objectModels[i]. Rebuilt every
    // frame in captureRenderFrame() from this frame's camera; the shadow
    // pass ignores it and uses objectMeshVisibleLight instead, since it
    // needs the light frustum, not the camera one this was built from.
    std::vector<std::vector<uint8_t>> objectMeshVisible;

    // The clusters each visible mesh draws in the camera passes, made once
    // per frame in captureRenderFrame(). The depth prepass and every pass
    // that tests against its depth must draw this same cut.
    sponge::scene::ClusterCut cameraCut;

    // Same shape as objectMeshVisible, but tested against each cascade's
    // frustum (lightSpaceMatrices) instead of the camera's. Only rebuilt when
    // the shadow pass will actually run; stale otherwise since it goes
    // unread.
    std::array<std::vector<std::vector<uint8_t>>,
               sponge::platform::opengl::scene::ShadowMap::maxCascades>
        objectMeshVisibleLight;

    // Per cascade: the clusters each mesh draws into that cascade, made from
    // the cascade's texel size only, so a cached cascade stays correct.
    // Shared and never changed after it is made: the update thread makes a
    // new one when the texel size changes. Null draws LOD0.
    std::array<std::shared_ptr<const sponge::scene::ClusterCut>,
               sponge::platform::opengl::scene::ShadowMap::maxCascades>
        shadowCuts;

    // settings.probeEnabled is false when the scene has no probe; the render
    // thread also waits for probeCaptured before it actually samples.

    // True when the planar mirror is drawn this frame.
    bool planarActive{ false };
    // Index of the planar mirror object; valid when planarActive.
    size_t planarIndex{ 0 };
    // Oblique projection times the view reflected in the mirror plane.
    glm::mat4 planarViewProj{ 1.F };
    // Camera position reflected in the mirror plane.
    glm::vec3 planarViewPos{ 0.F };
    // World-space mirror plane normal, for the composite pass to discard the
    // mirror mesh's side faces.
    glm::vec3 planarNormal{ 0.F, 1.F, 0.F };
    // Per-object visibility in the mirrored view, index-locked with
    // objectModels.
    std::vector<uint8_t> planarObjectVisible;
};

}  // namespace game::thread
