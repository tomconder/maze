#include "shadowmap.hpp"
#include "logging/log.hpp"
#include "platform/opengl/renderer/gl.hpp"
#include "platform/opengl/renderer/resourcemanager.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace sponge::platform::opengl::scene {
ShadowMap::ShadowMap() {
    shader = renderer::ResourceManager::loadShader(
        shaderName, "/shaders/shadowmap.vert.glsl",
        "/shaders/shadowmap.frag.glsl");
    shader->bind();

    depthMap = std::make_unique<renderer::Texture>();
    depthMap->createDepthMap(SHADOW_WIDTH, SHADOW_HEIGHT);

    framebuffer = std::make_unique<renderer::FrameBuffer>();

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           depthMap->getId(), 0);
    if (!renderer::FrameBuffer::checkStatus()) {
        SPONGE_CORE_CRITICAL("Framebuffer is not complete!");
        return;
    }

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    framebuffer->unbind();

    shader->unbind();
}

void ShadowMap::bind() const {
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    framebuffer->bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    depthMap->activateAndBind(0);
}

void ShadowMap::unbind() const {
    framebuffer->unbind();
}

void ShadowMap::activateAndBindDepthMap(const uint8_t unit) const {
    depthMap->activateAndBind(unit);
}

void ShadowMap::updateLightSpaceMatrix(const glm::vec3& lightPos) {
    const auto lightProjection =
        glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, nearPlane, farPlane);

    const auto lightView =
        glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    lightSpaceMatrix = lightProjection * lightView;
}
}  // namespace sponge::platform::opengl::scene
