#pragma once

#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"

#include <cstdint>
#include <memory>
#include <string_view>

namespace sponge::platform::opengl::scene {

// The linear HDR buffer the scene renders into, and the pass that turns it
// into a display image.
//
// The two belong together: everything upstream of resolve() works in unbounded
// radiance, and resolve() is the single point where the pipeline leaves linear
// space. Bloom extracts from getTexture() before that happens, so its threshold
// is a radiance value rather than a point on an already-compressed curve.
class SceneTarget {
public:
    SceneTarget() = delete;
    SceneTarget(uint32_t width, uint32_t height);
    ~SceneTarget();

    SceneTarget(const SceneTarget&)            = delete;
    SceneTarget& operator=(const SceneTarget&) = delete;

    void begin() const;
    void end() const;

    // Linear HDR scene colour. Valid after end().
    uint32_t getTexture() const {
        return colorTexture;
    }

    // Composites bloom in linear light, then tone maps and gamma encodes into
    // whichever framebuffer is currently bound. `bloomTexId` may be 0 when
    // `bloomIntensity` is 0. Set `ditherOutput` only when this pass writes the
    // final 8-bit target — anti-aliasing, when enabled, runs afterwards and
    // dithers on its own write.
    void resolve(uint32_t bloomTexId, float bloomIntensity,
                 bool ditherOutput) const;

    void resize(uint32_t newWidth, uint32_t newHeight);

private:
    static constexpr std::string_view shaderName = "tonemap";

    std::shared_ptr<renderer::Shader>       shader;
    std::unique_ptr<renderer::VertexArray>  vao;
    std::unique_ptr<renderer::VertexBuffer> vbo;

    // Raw GL handles, as in the other post-processing classes: the Texture
    // class has no colour-buffer creation path and these are rebuilt on resize.
    uint32_t colorTexture = 0;
    uint32_t depthRbo     = 0;
    uint32_t fbo          = 0;

    uint32_t width  = 0;
    uint32_t height = 0;

    void initialize();
    void createFramebuffer();
    void destroyFramebuffer();
};

}  // namespace sponge::platform::opengl::scene
