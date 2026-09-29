#include "platform/opengl/renderer/ssbo.hpp"

#include "platform/opengl/renderer/gl.hpp"

namespace sponge::platform::opengl::renderer {

SSBO::SSBO(const std::size_t bytes) {
    glCreateBuffers(1, &id);
    glNamedBufferStorage(id, static_cast<GLsizeiptr>(bytes), nullptr,
                         GL_DYNAMIC_STORAGE_BIT);
}

SSBO::SSBO(SSBO&& other) noexcept {
    id       = other.id;
    other.id = 0;
}

SSBO& SSBO::operator=(SSBO&& other) noexcept {
    if (this != &other) {
        if (id != 0) {
            glDeleteBuffers(1, &id);
        }
        id       = other.id;
        other.id = 0;
    }
    return *this;
}

SSBO::~SSBO() {
    if (id != 0) {
        glDeleteBuffers(1, &id);
    }
}

void SSBO::update(const void* data, const std::size_t bytes) const {
    glNamedBufferSubData(id, 0, static_cast<GLsizeiptr>(bytes), data);
}

void SSBO::bindBase(const uint32_t binding) const {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, id);
}

}  // namespace sponge::platform::opengl::renderer
