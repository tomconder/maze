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

    glGenQueries(static_cast<GLsizei>(objectCount), queries.data());

    shader = AssetManager::createShader({
        .name           = "occlusionbox",
        .vertexShader   = "occlusionbox.vert",
        .fragmentShader = "occlusionbox.frag",
    });
    shader->bind();

    vao = std::make_unique<renderer::VertexArray>();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(unitCubeVertices.data(),
                                                   sizeof(unitCubeVertices));
    vbo->bind();

    constexpr uint32_t positionLoc = 0;
    glEnableVertexAttribArray(positionLoc);
    glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                          reinterpret_cast<const void*>(0));

    shader->unbind();
    vao->unbind();
}

OcclusionCuller::~OcclusionCuller() {
    if (!queries.empty()) {
        glDeleteQueries(static_cast<GLsizei>(queries.size()), queries.data());
    }
}

void OcclusionCuller::query(const std::vector<sponge::scene::AABB>& worldBounds,
                            const glm::mat4&                        viewProj,
                            const glm::vec3&                        cameraPos) {
    shader->bind();
    vao->bind();

    for (size_t i = 0; i < worldBounds.size(); i++) {
        const auto& box = worldBounds[i];

        const bool cameraInside =
            cameraPos.x >= box.min.x && cameraPos.x <= box.max.x &&
            cameraPos.y >= box.min.y && cameraPos.y <= box.max.y &&
            cameraPos.z >= box.min.z && cameraPos.z <= box.max.z;
        if (cameraInside) {
            visible[i] = 1;
            // Leave unissued: pollResults() then skips it until a query
            // fired after the camera leaves the box gives a real answer.
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

    vao->unbind();
    shader->unbind();
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
