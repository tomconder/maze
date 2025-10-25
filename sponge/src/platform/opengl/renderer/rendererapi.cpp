#include "platform/opengl/renderer/rendererapi.hpp"

#include "platform/opengl/renderer/errorhandler.hpp"
#include "platform/opengl/renderer/gl.hpp"

namespace sponge::platform::opengl::renderer {

void RendererAPI::init() {
    if (glDebugMessageCallback != nullptr) {
        glDebugMessageCallback(glLogMessage, nullptr);
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        // do not want driver notification spam
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
                              GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr,
                              GL_FALSE);
    }

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

void RendererAPI::setClearColor(const glm::vec4& color) {
    glClearColor(color.r, color.g, color.b, color.a);
}

void RendererAPI::clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

}  // namespace sponge::platform::opengl::renderer
