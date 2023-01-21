#include "openglrendererapi.h"

void OpenGLRendererAPI::init() {
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

void OpenGLRendererAPI::setViewport(int32_t x, int32_t y, int32_t width, int32_t height) {
    glViewport(x, y, width, height);
}

void OpenGLRendererAPI::setClearColor(const glm::vec4 &color) {
    glClearColor(color.r, color.g, color.b, color.a);
}

void OpenGLRendererAPI::clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
