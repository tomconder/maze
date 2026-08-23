#pragma once

#include "core/base.hpp"

#include <glad/gl.h>

#include <cstdint>

namespace sponge::platform::opengl::renderer {

// FBO attachment: allocate, upload nothing, no mip chain. Wrap is always
// CLAMP_TO_EDGE — every sampler reading one of these reads neighbouring
// texels, and wrapping would pull in the opposite edge of the screen.
// Callers needing another wrap (EVSM's border-colour moment map) rebind and
// override.
inline uint32_t createRenderTarget(const uint32_t width, const uint32_t height,
                                   const GLint  internalFormat,
                                   const GLenum format, const GLenum type,
                                   const GLint filter) {
    uint32_t id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, static_cast<GLsizei>(width),
                 static_cast<GLsizei>(height), 0, format, type, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

}  // namespace sponge::platform::opengl::renderer
