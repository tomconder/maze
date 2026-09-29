#include "platform/opengl/scene/occlusionculler.hpp"

#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"
#include "platform/opengl/scene/unitcube.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

OcclusionCuller::OcclusionCuller(const size_t objectCount) :
    queries(objectCount, 0), visible(objectCount, 1), issued(objectCount, 0) {
    if (objectCount == 0) {
        return;
    }

    glCreateQueries(GL_ANY_SAMPLES_PASSED, static_cast<GLsizei>(objectCount),
                    queries.data());

    shader = AssetManager::createShader({
        .name           = "occlusionbox",
        .vertexShader   = "occlusionbox.vert",
        .fragmentShader = "occlusionbox.frag",
    });
    vao    = std::make_unique<renderer::VertexArray>();
    vbo    = std::make_unique<renderer::VertexBuffer>(unitCubeVertices.data(),
                                                      sizeof(unitCubeVertices));
    vao->setVertexBuffer(*vbo, sizeof(glm::vec3));

    constexpr uint32_t positionLoc = 0;
    vao->addAttribute(positionLoc, 3, 0);
}

OcclusionCuller::~OcclusionCuller() {
    if (!queries.empty()) {
        glDeleteQueries(static_cast<GLsizei>(queries.size()), queries.data());
    }
}

void OcclusionCuller::query(const std::vector<sponge::scene::AABB>& worldBounds,
                            const glm::mat4&                        viewProj,
                            const glm::vec3&                        eyePos) {
    // Depth-test only: no colour, no depth write, so this can't perturb the
    // depth buffer it tests against.
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    // Global back-face culling (RendererAPI) would cull every face of a box
    // the camera is inside — true for any large object's AABB, e.g. sponza's
    // — leaving the query with nothing to rasterize and the object stuck
    // permanently "occluded". The proxy has no back faces to hide anyway.
    glDisable(GL_CULL_FACE);

    shader->bind();
    vao->bind();

    // A tight-fitting mesh's own AABB sits at almost exactly its rendered
    // depth, so GL_LEQUAL against the depth just rasterized for that same
    // object is a coin flip between the proxy's and the mesh's independently
    // built matrices — worst up close, where depth precision stops rounding
    // the noise away. Bias the proxy toward the camera so it reliably reads
    // as visible against its own geometry without masking real occluders.
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.F, -1.F);

    for (size_t i = 0; i < worldBounds.size(); i++) {
        const auto& box = worldBounds[i];

        const bool eyeInside = eyePos.x >= box.min.x && eyePos.x <= box.max.x &&
                               eyePos.y >= box.min.y && eyePos.y <= box.max.y &&
                               eyePos.z >= box.min.z && eyePos.z <= box.max.z;
        if (eyeInside) {
            visible[i] = 1;
            // Leave unissued: pollResults() then skips it until a query
            // fired after the eye leaves the box gives a real answer.
            issued[i] = 0;
            continue;
        }

        const auto center   = (box.min + box.max) * 0.5F;
        const auto size     = box.max - box.min;
        const auto boxModel = glm::translate(glm::mat4(1.F), center) *
                              glm::scale(glm::mat4(1.F), size);
        shader->setMat4("mvp", viewProj * boxModel);

        glBeginQuery(GL_ANY_SAMPLES_PASSED, queries[i]);
        glDrawArrays(GL_TRIANGLES, 0, unitCubeVertexCount);
        glEndQuery(GL_ANY_SAMPLES_PASSED);
        issued[i] = 1;
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    vao->unbind();
    shader->unbind();

    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void OcclusionCuller::pollResults() {
    for (size_t i = 0; i < queries.size(); i++) {
        if (!issued[i]) {
            continue;
        }

        GLuint available = 0;
        glGetQueryObjectuiv(queries[i], GL_QUERY_RESULT_AVAILABLE, &available);
        if (available == GL_FALSE) {
            continue;
        }
        GLuint anyPassed = 0;
        glGetQueryObjectuiv(queries[i], GL_QUERY_RESULT, &anyPassed);
        visible[i] = anyPassed != 0U ? 1 : 0;
    }
}

}  // namespace sponge::platform::opengl::scene
