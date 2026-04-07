#include "platform/opengl/scene/fxaa.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <array>

namespace {
// Interleaved NDC position (vec2) + UV (vec2) for a full-screen triangle strip.
// Triangle strip chosen over two separate triangles to halve the vertex count
// and avoid a diagonal seam that can produce a one-pixel artifact on some GPUs.
constexpr std::array quadVertices = {
    -1.F, -1.F, 0.F, 0.F, 1.F, -1.F, 1.F, 0.F,
    -1.F, 1.F,  0.F, 1.F, 1.F, 1.F,  1.F, 1.F,
};

constexpr uint32_t quadVertexCount = 4;
constexpr uint32_t quadStride      = 4 * sizeof(float);
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

FXAA::FXAA(const uint32_t width, const uint32_t height) :
    width(width), height(height) {
    initialize();
}

FXAA::~FXAA() {
    destroyFramebuffer();
}

void FXAA::initialize() {
    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name               = shaderName.data(),
        .vertexShaderPath   = "/shaders/fxaa.vert.glsl",
        .fragmentShaderPath = "/shaders/fxaa.frag.glsl",
    };
    shader = AssetManager::createShader(shaderCreateInfo);
    shader->bind();

    vao = renderer::VertexArray::create();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(quadVertices.data(),
                                                   sizeof(quadVertices));
    vbo->bind();

    const auto program = shader->getId();

    if (const auto loc = glGetAttribLocation(program, "position"); loc != -1) {
        const auto pos = static_cast<uint32_t>(loc);
        glEnableVertexAttribArray(pos);
        glVertexAttribPointer(pos, 2, GL_FLOAT, GL_FALSE, quadStride,
                              reinterpret_cast<const void*>(0));
    }

    if (const auto loc = glGetAttribLocation(program, "texCoord"); loc != -1) {
        const auto tc = static_cast<uint32_t>(loc);
        glEnableVertexAttribArray(tc);
        glVertexAttribPointer(tc, 2, GL_FLOAT, GL_FALSE, quadStride,
                              reinterpret_cast<const void*>(2 * sizeof(float)));
    }

    shader->setInteger("screenTexture", 0);

    shader->unbind();
    vao->unbind();

    createFramebuffer();
}

void FXAA::createFramebuffer() {
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, static_cast<GLsizei>(width),
                 static_cast<GLsizei>(height), 0, GL_RGB, GL_UNSIGNED_BYTE,
                 nullptr);

    // Linear filtering is required — FXAA samples between texels when blending
    // across detected edges, so nearest-neighbor would negate the entire pass.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Depth renderbuffer rather than a depth texture because the FXAA pass
    // never samples depth — renderbuffers are faster for write-only
    // attachments.
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
        SPONGE_GL_CRITICAL("FXAA framebuffer is not complete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FXAA::destroyFramebuffer() {
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

void FXAA::begin() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void FXAA::end() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FXAA::apply() const {
    // Depth testing would discard full-screen quad fragments behind anything
    // left in the depth buffer from the scene pass — disable for this blit.
    glDisable(GL_DEPTH_TEST);

    shader->bind();
    shader->setFloat2("rcpFrame", glm::vec2(1.0f / static_cast<float>(width),
                                            1.0f / static_cast<float>(height)));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTexture);

    vao->bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, quadVertexCount);
    vao->unbind();

    shader->unbind();

    glEnable(GL_DEPTH_TEST);
}

void FXAA::resize(const uint32_t newWidth, const uint32_t newHeight) {
    if (width == newWidth && height == newHeight) {
        return;
    }
    width  = newWidth;
    height = newHeight;
    destroyFramebuffer();
    createFramebuffer();
}

}  // namespace sponge::platform::opengl::scene
