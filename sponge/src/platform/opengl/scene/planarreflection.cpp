#include "platform/opengl/scene/planarreflection.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/gl.hpp"

namespace sponge::platform::opengl::scene {

PlanarReflection::PlanarReflection(const uint32_t width,
                                   const uint32_t height) :
    width(width), height(height) {
    create();
}

PlanarReflection::~PlanarReflection() {
    destroy();
}

void PlanarReflection::resize(const uint32_t width, const uint32_t height) {
    this->width  = width;
    this->height = height;
    destroy();
    create();
}

void PlanarReflection::create() {
    colorTexture = renderer::createRenderTarget(width, height, GL_RGB16F,
                                                GL_RGB, GL_FLOAT, GL_LINEAR);

    glGenRenderbuffers(1, &depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                          static_cast<GLsizei>(width),
                          static_cast<GLsizei>(height));
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           colorTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depthRbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SPONGE_GL_CRITICAL("Planar reflection framebuffer is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PlanarReflection::destroy() {
    if (fbo != 0) {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
    if (colorTexture != 0) {
        glDeleteTextures(1, &colorTexture);
        colorTexture = 0;
    }
    if (depthRbo != 0) {
        glDeleteRenderbuffers(1, &depthRbo);
        depthRbo = 0;
    }
}

void PlanarReflection::begin() const {
    glGetIntegerv(GL_VIEWPORT, savedViewport.data());
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glFrontFace(GL_CW);
}

void PlanarReflection::end() const {
    glFrontFace(GL_CCW);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(savedViewport[0], savedViewport[1], savedViewport[2],
               savedViewport[3]);
}

glm::mat4 PlanarReflection::reflectionMatrix(const glm::vec4& plane) {
    const glm::vec3 n(plane);
    const float     d = plane.w;
    // Column-major: r[column][row]. The 3x3 part is I - 2 n n^T.
    glm::mat4 r(1.F);
    for (int c = 0; c < 3; c++) {
        for (int row = 0; row < 3; row++) {
            r[c][row] -= 2.F * n[row] * n[c];
        }
        r[3][c] = -2.F * d * n[c];
    }
    return r;
}

std::optional<glm::mat4>
    PlanarReflection::obliqueProjection(const glm::mat4& projection,
                                        const glm::vec4& viewPlane) {
    // Lengyel's method with m[column][row]: q is the frustum corner opposite
    // the plane in clip space, and the third row becomes the scaled plane.
    glm::mat4       m = projection;
    const glm::vec4 q((glm::sign(viewPlane.x) + m[2][0]) / m[0][0],
                      (glm::sign(viewPlane.y) + m[2][1]) / m[1][1], -1.F,
                      (1.F + m[2][2]) / m[3][2]);
    const float     dq = glm::dot(viewPlane, q);
    if (dq <= 1e-6F) {
        return std::nullopt;
    }
    const glm::vec4 c = viewPlane * (2.F / dq);
    m[0][2]           = c.x;
    m[1][2]           = c.y;
    m[2][2]           = c.z + 1.F;
    m[3][2]           = c.w;
    return m;
}

}  // namespace sponge::platform::opengl::scene
