#pragma once

#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/scene/screenquad.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace sponge::platform::opengl::scene {

// The scene seen from one point, as a cubemap. The caller draws each face
// between beginFace() and end(), then prefilter() convolves the capture with
// GGX for every roughness level. Surfaces sample getTexture() at mip
// roughness * (mipLevels - 1).
//
// A second capture of the scene on a black background, the bounce capture,
// is convolved with a cosine lobe into a small cubemap, getBounceIrradiance():
// the light the scene's own surfaces send back, with no sky in it. A diffuse
// surface samples it along its normal. Draw its six faces with
// beginFace(face, Capture::Bounce) as well, before prefilter().
//
// A third cube holds the sky alone, the environment image with no scene (a
// flat grey when there is none). Its irradiance, getSkyIrradiance(), is the
// light the whole open sky sends to a surface with that normal, before any
// occlusion. Draw its six faces with beginFace(face, Capture::Sky) and
// end(), with nothing between.
class ReflectionProbe {
public:
    static constexpr uint32_t faceSize  = 256;
    static constexpr int      mipLevels = 6;
    // Irradiance is smooth, so a few texels per face are enough.
    static constexpr uint32_t irradianceSize = 32;

    // environment is a baked half-float equirect image (see the manifest's
    // environments section). Empty, or a file that does not load, leaves the
    // capture's background a flat grey.
    explicit ReflectionProbe(const std::string& environment);
    ~ReflectionProbe();

    ReflectionProbe(const ReflectionProbe&)            = delete;
    ReflectionProbe& operator=(const ReflectionProbe&) = delete;

    // Binds face (0..5, GL order +X -X +Y -Y +Z -Z) of the capture cubemap,
    // fills it with the environment image (linear radiance, what shows
    // through openings in the scene), and saves the caller's viewport.
    // Capture::Bounce binds the bounce capture instead and clears it black,
    // and Capture::Sky binds the sky-only cube and fills it like Scene does.
    // end() binds framebuffer 0 and restores the viewport.
    enum class Capture : uint8_t { Scene, Bounce, Sky };
    void beginFace(int face, Capture capture = Capture::Scene) const;
    void end() const;

    // The scene seen from straight above, as a map of the highest surface
    // (world y) at each point of the box [boxMin, boxMax] in x and z. Row 0 is
    // boxMax.z, so uv = ((x - minX) / sizeX, 1 - (z - minZ) / sizeZ).
    // beginSkyHeight() binds it for drawing with getSkyHeightShader(), which
    // takes `viewProj` (skyHeightViewProj()) and `model`; the caller draws the
    // opaque meshes and calls end(). y limits the depth range, so geometry
    // outside [minY, maxY] is clipped away. Cells nothing covers read minY - 1.
    static constexpr uint32_t skyHeightSize = 1024;
    static glm::mat4 skyHeightViewProj(const glm::vec3& boxMin,
                                       const glm::vec3& boxMax, float minY,
                                       float maxY);
    void             beginSkyHeight(float minY) const;

    std::shared_ptr<renderer::Shader> getSkyHeightShader() const {
        return skyHeightShader;
    }
    uint32_t getSkyHeight() const {
        return skyHeightTexture;
    }

    // Fills every mip level of the result cubemap and the irradiance cubemaps
    // from the capture. Call after all six faces are drawn.
    void prefilter() const;

    uint32_t getTexture() const {
        return resultCube;
    }

    uint32_t getBounceIrradiance() const {
        return bounceIrradianceCube;
    }

    uint32_t getSkyIrradiance() const {
        return skyIrradianceCube;
    }

    // View matrix for a GL cube face at position, with the up vectors the
    // GL cube map layout expects. Use with a 90 degree, aspect 1 projection.
    static glm::mat4 faceView(const glm::vec3& position, int face);

private:
    uint32_t                          fbo                  = 0;
    uint32_t                          depthRbo             = 0;
    uint32_t                          captureCube          = 0;
    uint32_t                          resultCube           = 0;
    uint32_t                          bounceCube           = 0;
    uint32_t                          bounceIrradianceCube = 0;
    uint32_t                          skyCube              = 0;
    uint32_t                          skyIrradianceCube    = 0;
    uint32_t                          skyHeightTexture     = 0;
    uint32_t                          skyHeightFbo         = 0;
    uint32_t                          skyHeightDepth       = 0;
    std::shared_ptr<renderer::Shader> shader;
    std::shared_ptr<renderer::Shader> irradianceShader;
    std::shared_ptr<renderer::Shader> skyShader;
    std::shared_ptr<renderer::Shader> skyHeightShader;
    // Null when there is no usable environment image.
    std::shared_ptr<renderer::Texture> environment;
    ScreenQuad                         quad;
    mutable std::array<int32_t, 4>     savedViewport{};

    // Cosine-convolves a capture cube (its mip chain already built) into a
    // small irradiance cube. Expects prefilter()'s framebuffer state.
    void convolveIrradiance(uint32_t source, uint32_t target) const;
};

}  // namespace sponge::platform::opengl::scene
