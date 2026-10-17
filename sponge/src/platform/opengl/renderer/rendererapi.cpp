#include "platform/opengl/renderer/rendererapi.hpp"

#include "debug/profiler.hpp"
#include "platform/opengl/renderer/errorhandler.hpp"
#include "platform/opengl/renderer/gl.hpp"

namespace sponge::platform::opengl::renderer {

void RendererAPI::init() const {
    // Release builds have no debug context, and synchronous output can cost
    // the driver its threaded submission.
#ifndef NDEBUG
    if (glDebugMessageCallback != nullptr) {
        glDebugMessageCallback(glLogMessage, nullptr);
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        // do not want driver notification spam
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
                              GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr,
                              GL_FALSE);
    }
#endif

    glEnable(GL_MULTISAMPLE);

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
}

void RendererAPI::setViewport(const int32_t x, const int32_t y,
                              const int32_t width, const int32_t height) {
    glViewport(x, y, width, height);
}

void RendererAPI::setClearColor(const glm::vec4& color) const {
    glClearColor(color.r, color.g, color.b, color.a);
}

void RendererAPI::clear(const bool clearColour) const {
    glClear((clearColour ? GL_COLOR_BUFFER_BIT : 0) | GL_DEPTH_BUFFER_BIT);
}

void RendererAPI::clearColor() {
    glClear(GL_COLOR_BUFFER_BIT);
}

void RendererAPI::setDepth(const DepthFunc func, const bool write) {
    glDepthFunc(func == DepthFunc::Less ? GL_LESS : GL_LEQUAL);
    glDepthMask(write ? GL_TRUE : GL_FALSE);
}

void RendererAPI::setAlphaBlend(const bool enabled) {
    if (enabled) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
}

void RendererAPI::setPremultipliedAlphaBlend() {
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
}

void RendererAPI::setScissor(const int32_t x, const int32_t y,
                             const int32_t width, const int32_t height) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, width, height);
}

void RendererAPI::disableScissor() {
    glDisable(GL_SCISSOR_TEST);
}

void RendererAPI::bindTexture(const uint32_t unit, const uint32_t texture) {
    glBindTextureUnit(unit, texture);
}

void RendererAPI::flush() {
    SPONGE_PROFILE_SECTION("glFlush");
    glFlush();
}

void RendererAPI::pushGroup(const std::string_view name) {
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0,
                     static_cast<GLsizei>(name.size()), name.data());
}

void RendererAPI::popGroup() {
    glPopDebugGroup();
}

}  // namespace sponge::platform::opengl::renderer
