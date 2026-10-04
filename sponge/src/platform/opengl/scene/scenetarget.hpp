#pragma once

#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/scene/screenquad.hpp"

#include <cstdint>
#include <memory>
#include <string_view>

namespace sponge::platform::opengl::scene {

// Order must match the `tone*` constants in tonemap.slang.
enum class ToneMapper : uint8_t {
    Reinhard = 0,
    Filmic,
    Aces,
    Agx,
    Count,
};

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

    // Copy the scene color into a texture the glass pass can sample. The
    // copy is not an attachment of the framebuffer being drawn.
    uint32_t copyColor() const;

    // Bind the glass framebuffer: scene color, the caller's velocity
    // texture, and this target's depth renderbuffer. The renderbuffer
    // already holds the blitted opaque depth. Returns false when the
    // framebuffer is incomplete; the caller skips the draw.
    //
    // The prepass depth texture is sampled by the shader and is not
    // attached here. Sampling a texture that is also the bound depth
    // attachment is undefined.
    bool beginGlass(uint32_t velocityTex, bool writeVelocity) const;

    void endGlass() const;

    // Blit this target's depth renderbuffer onto destFbo's depth attachment.
    void blitDepthTo(uint32_t destFbo, int width, int height) const;

    // Composites bloom in linear light, then tone maps and gamma encodes into
    // whichever framebuffer is currently bound. `bloomTexId` may be 0 when
    // `bloomIntensity` is 0. Set `ditherOutput` only when this pass writes the
    // final 8-bit target — anti-aliasing, when enabled, runs afterwards and
    // dithers on its own write.
    void resolve(uint32_t bloomTexId, float bloomIntensity, bool ditherOutput,
                 ToneMapper toneMapper) const;

    void resize(uint32_t newWidth, uint32_t newHeight);

private:
    static constexpr std::string_view shaderName = "tonemap";

    std::shared_ptr<renderer::Shader> shader;
    ScreenQuad                        quad;

    // Raw GL handles, as in the other post-processing classes: the Texture
    // class has no colour-buffer creation path and these are rebuilt on resize.
    uint32_t colorTexture = 0;
    uint32_t colorCopy    = 0;
    uint32_t depthRbo     = 0;
    uint32_t fbo          = 0;
    uint32_t glassFbo     = 0;

    uint32_t width  = 0;
    uint32_t height = 0;

    void initialize();
    void createFramebuffer();
    void destroyFramebuffer();
};

}  // namespace sponge::platform::opengl::scene
