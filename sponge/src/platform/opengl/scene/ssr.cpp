#include "platform/opengl/scene/ssr.hpp"

#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

Ssr::Ssr() {
    shader = AssetManager::createShader(renderer::ShaderCreateInfo{
        .name           = "ssr",
        .vertexShader   = "screenquad.vert",
        .fragmentShader = "ssr.frag",
    });
}

void Ssr::apply(const uint32_t colorTexId, const uint32_t depthTexId,
                const uint32_t normalTexId, const glm::mat4& projection,
                const glm::mat4& invProjection) const {
    // The shader samples the scene depth, so the depth test would only reject
    // the full-screen quad. Blending stays on: the output alpha is the
    // reflection weight.
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    // The output alpha is the reflection weight, so this pass must not depend
    // on whatever blend function an earlier pass left set.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader->bind();
    shader->setMat4("projection", projection);
    shader->setMat4("invProjection", invProjection);
    glBindTextureUnit(0, colorTexId);
    glBindTextureUnit(1, depthTexId);
    glBindTextureUnit(2, normalTexId);
    quad.draw();
    shader->unbind();

    glEnable(GL_DEPTH_TEST);
}

}  // namespace sponge::platform::opengl::scene
