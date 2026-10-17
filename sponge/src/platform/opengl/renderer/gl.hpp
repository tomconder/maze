#pragma once

#include "logging/log.hpp"

#include <glad/gl.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

namespace sponge::platform::opengl::renderer {

// Names a GL object for RenderDoc, Nsight and driver messages. `identifier` is
// GL_TEXTURE, GL_FRAMEBUFFER, GL_RENDERBUFFER, GL_PROGRAM and so on. Does
// nothing in release builds. The name need not end in a null.
inline void labelObject([[maybe_unused]] const GLenum           identifier,
                        [[maybe_unused]] const uint32_t         id,
                        [[maybe_unused]] const std::string_view name) {
#ifndef NDEBUG
    glObjectLabel(identifier, id, static_cast<GLsizei>(name.size()),
                  name.data());
#endif
}

// FBO attachment: immutable storage, no mip chain. Wrap is always
// CLAMP_TO_EDGE — every sampler reading one of these reads neighbouring
// texels, and wrapping would pull in the opposite edge of the screen.
// Callers needing another wrap (EVSM's border-colour moment map) override it
// with glTextureParameter*. A minimised window reports a zero size, which
// immutable storage rejects, so each side is at least one texel.
inline uint32_t createRenderTarget(const uint32_t width, const uint32_t height,
                                   const GLenum internalFormat,
                                   const GLint  filter) {
    uint32_t id = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &id);
    glTextureStorage2D(id, 1, internalFormat,
                       static_cast<GLsizei>(std::max(width, 1U)),
                       static_cast<GLsizei>(std::max(height, 1U)));
    glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, filter);
    glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, filter);
    glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return id;
}

inline uint32_t createDepthRenderbuffer(const uint32_t width,
                                        const uint32_t height) {
    uint32_t id = 0;
    glCreateRenderbuffers(1, &id);
    glNamedRenderbufferStorage(id, GL_DEPTH_COMPONENT24,
                               static_cast<GLsizei>(width),
                               static_cast<GLsizei>(height));
    return id;
}

// One color attachment, plus a depth renderbuffer when depthRbo is not 0.
// An incomplete framebuffer is logged and still returned.
// The framebuffer and its attachments are labelled from `name`.
inline uint32_t createFramebuffer(const std::string_view name,
                                  const uint32_t         colorTexture,
                                  const uint32_t         depthRbo = 0) {
    uint32_t fbo = 0;
    glCreateFramebuffers(1, &fbo);
    labelObject(GL_FRAMEBUFFER, fbo, std::string(name) + " FBO");
    labelObject(GL_TEXTURE, colorTexture, std::string(name) + " color");
    if (depthRbo != 0) {
        labelObject(GL_RENDERBUFFER, depthRbo, std::string(name) + " depth");
    }
    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, colorTexture, 0);
    if (depthRbo != 0) {
        glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_ATTACHMENT,
                                       GL_RENDERBUFFER, depthRbo);
    }
    if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) !=
        GL_FRAMEBUFFER_COMPLETE) {
        SPONGE_GL_CRITICAL("{} framebuffer is not complete!", name);
    }
    return fbo;
}

}  // namespace sponge::platform::opengl::renderer
