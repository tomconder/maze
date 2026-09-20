#pragma once

#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"
#include "scene/frustum.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace sponge::platform::opengl::scene {

// Hardware occlusion culling: one GL_ANY_SAMPLES_PASSED query per object,
// its world AABB drawn depth-test-only against whatever is already in the
// bound depth buffer. Results lag one frame: call pollResults() at the
// start of a frame to pick up last frame's answers before deciding what to
// draw, then query() once this frame's real depth is rasterized, so the
// result is ready for next frame. An object stays visible until its first
// result arrives, so nothing is hidden before there's data to hide it with.
class OcclusionCuller {
public:
    explicit OcclusionCuller(size_t objectCount);
    ~OcclusionCuller();

    OcclusionCuller(const OcclusionCuller&)            = delete;
    OcclusionCuller& operator=(const OcclusionCuller&) = delete;

    // Caller must already have the target depth buffer bound, with depth
    // test on (GL_LEQUAL) and colour/depth writes off — this only reads
    // depth. worldBounds must be objectCount long, index-locked with the
    // caller's own object list. viewProj/eyePos are whatever's being tested
    // against — a perspective camera or an orthographic light.
    //
    // An object whose box contains eyePos is skipped and left visible. This
    // matters for a perspective eye inside a convex box: only the far face
    // rasterizes (the near face is behind the eye, clipped away), and
    // that's always farther than the object's own nearby geometry already
    // in the depth buffer, so the query would always "fail" against itself
    // — true for any object large enough to enclose the eye (e.g. the whole
    // level). An orthographic eye (e.g. a directional light) doesn't clip
    // this way, so there the guard is just a harmless no-op.
    void query(const std::vector<sponge::scene::AABB>& worldBounds,
               const glm::mat4& viewProj, const glm::vec3& eyePos);

    // Non-blocking: pulls in whichever queries already have a result.
    // Objects with no result yet keep their last known visibility.
    void pollResults();

    bool isVisible(size_t index) const {
        return visible[index] != 0;
    }

private:
    std::shared_ptr<renderer::Shader>       shader;
    std::unique_ptr<renderer::VertexArray>  vao;
    std::unique_ptr<renderer::VertexBuffer> vbo;
    std::vector<uint32_t>                   queries;
    std::vector<uint8_t>                    visible;
    // A name from glGenQueries isn't a valid query object until it's been
    // through one glBeginQuery/glEndQuery pair — polling it before that is a
    // GL_INVALID_OPERATION ("query object not found"), so pollResults()
    // skips any index query() hasn't issued yet.
    std::vector<uint8_t> issued;
};

}  // namespace sponge::platform::opengl::scene
