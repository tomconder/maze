#pragma once

#include <glm/glm.hpp>

#include <array>
#include <string>
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
    glm::vec3 cameraPos{ 0.F };

    // Directional light / shadow
    bool      shadowEnabled{ false };
    bool      shadowCastShadow{ false };
    glm::vec3 lightDirection{ 0.F, -1.F, 0.F };
    glm::mat4 lightSpaceMatrix{ 1.F };

    // Point lights (up to 6)
    int32_t                  numLights{ 0 };
    std::array<glm::vec3, 6> lightPositions{};
    std::array<glm::vec3, 6> lightColors{};
    std::array<int32_t, 6>   lightAttenuationIndices{};

    // Game objects (model matrices and names for AssetManager lookup)
    std::vector<glm::mat4>   objectModelMatrices;
    std::vector<std::string> objectNames;

    // Post-processing
    bool fxaaEnabled{ false };
};

}  // namespace game::thread
