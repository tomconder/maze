#pragma once

#include "platform/opengl/scene/clusteredlights.hpp"
#include "platform/opengl/scene/model.hpp"

#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <vector>

namespace game::thread {

// Immutable snapshot of all rendering-relevant game state for one frame.
//
// The update thread fills this struct at the end of onUpdate() via
// captureRenderFrame(). The render thread reads from it in onRender().
// Two slots (indexed by frameIndex % 2) allow the next update thread to
// write slot N+1 while the render thread is reading slot N — no locks needed
// because the synchronization in UpdateThread/RenderThread ensures the writer
// and reader are always in different slots at any given moment.
struct MazeRenderFrame {
    // Camera
    glm::mat4 cameraMVP{ 1.F };
    glm::mat4 cameraView{ 1.F };
    glm::mat4 cameraProjection{ 1.F };
    glm::vec3 cameraPos{ 0.F };
    float     nearPlane{ 0.1F };
    float     farPlane{ 100.F };
    int       screenWidth{ 0 };
    int       screenHeight{ 0 };

    // Directional light / shadow
    bool      shadowEnabled{ false };
    bool      shadowCastShadow{ false };
    glm::vec3 lightDirection{ 0.F, -1.F, 0.F };
    glm::mat4 lightSpaceMatrix{ 1.F };

    static constexpr size_t maxLights = static_cast<size_t>(
        sponge::platform::opengl::scene::ClusteredLights::maxLights);

    // Point lights (one attenuation index shared by all)
    int32_t                          numLights{ 0 };
    int32_t                          lightAttenuationIndex{ 0 };
    std::array<glm::vec3, maxLights> lightPositions{};
    std::array<glm::vec3, maxLights> lightColors{};

    // Game objects: model matrices and resolved model handles. Handles are
    // resolved once at load; no per-frame name lookup.
    std::vector<glm::mat4> objectModelMatrices;
    std::vector<glm::vec3> objectEmissives;
    std::vector<std::shared_ptr<sponge::platform::opengl::scene::Model>>
        objectModels;

    // Post-processing
    bool  fxaaEnabled{ false };
    bool  bloomEnabled{ false };
    float bloomThreshold{ 0.8F };
    float bloomIntensity{ 2.5F };
};

}  // namespace game::thread
