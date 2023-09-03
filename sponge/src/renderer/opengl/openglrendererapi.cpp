#include "renderer/opengl/openglrendererapi.h"

static void APIENTRY glLogMessage(GLenum source, GLenum type, GLuint id,
                                  GLenum severity, GLsizei length,
                                  const GLchar* message,
                                  const void* userParam) {
    std::string source_str;
    std::string type_str;

    UNUSED(id);
    UNUSED(length);
    UNUSED(message);
    UNUSED(userParam);

    switch (source) {
        case GL_DEBUG_SOURCE_API:
            source_str = "API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            source_str = "WINDOW SYSTEM";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            source_str = "SHADER COMPILER";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            source_str = "THIRD PARTY";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            source_str = "APPLICATION";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            source_str = "OTHER";
            break;
        default:
            source_str = "UNKNOWN";
            break;
    }

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            type_str = "ERROR";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            type_str = "DEPRECATED BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            type_str = "UNDEFINED BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            type_str = "PORTABILITY";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            type_str = "PERFORMANCE";
            break;
        case GL_DEBUG_TYPE_OTHER:
            type_str = "OTHER";
            break;
        case GL_DEBUG_TYPE_MARKER:
            type_str = "MARKER";
            break;
        default:
            type_str = "UNKNOWN";
            break;
    }

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            SPONGE_GL_ERROR("{} {} [{}]: {}", source_str, type_str, id,
                            message);
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            SPONGE_GL_WARN("{} {} [{}]: {}", source_str, type_str, id, message);
            break;
        case GL_DEBUG_SEVERITY_LOW:
            SPONGE_GL_INFO("{} {} [{}]: {}", source_str, type_str, id, message);
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            SPONGE_GL_DEBUG("{} {} [{}]: {}", source_str, type_str, id, message);
            break;
        default:
            SPONGE_GL_TRACE("{} {} [{}]: {}", source_str, type_str, id, message);
            break;
    }
}

namespace sponge {

void OpenGLRendererAPI::init() {
    if (glDebugMessageCallback != nullptr) {
        glDebugMessageCallback(glLogMessage, nullptr);
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        // do not want driver notification spam
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
                              GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr,
                              GL_FALSE);
    }

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

void OpenGLRendererAPI::setViewport(int32_t x, int32_t y, int32_t width,
                                    int32_t height) {
    glViewport(x, y, width, height);
}

void OpenGLRendererAPI::setClearColor(const glm::vec4& color) {
    glClearColor(color.r, color.g, color.b, color.a);
}

void OpenGLRendererAPI::clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

}  // namespace sponge
