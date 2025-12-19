#include "platform/opengl/renderer/framebuffer.hpp"

#include "platform/opengl/renderer/gl.hpp"

namespace sponge::platform::opengl::renderer {

FrameBuffer::FrameBuffer() {
    glGenFramebuffers(1, &id);
    glBindFramebuffer(GL_FRAMEBUFFER, id);
}

FrameBuffer::FrameBuffer(FrameBuffer&& other) noexcept {
    id       = other.id;
    other.id = 0;
}

FrameBuffer& FrameBuffer::operator=(FrameBuffer&& other) noexcept {
    if (this != &other) {
        if (id != 0) {
            glDeleteFramebuffers(1, &id);
        }
        id       = other.id;
        other.id = 0;
    }
    return *this;
}

FrameBuffer::~FrameBuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &id);
}

void FrameBuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, id);
}

void FrameBuffer::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool FrameBuffer::checkStatus() {
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return false;
    }
    return true;
}

}  // namespace sponge::platform::opengl::renderer
