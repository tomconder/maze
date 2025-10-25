#include "platform/opengl/renderer/errorhandler.hpp"

#include "core/base.hpp"
#include "glad/gl.h"
#include "logging/log.hpp"

#include <string>

void GLAPIENTRY glLogMessage(GLenum source, GLenum type, uint32_t id,
                             GLenum severity, GLsizei length,
                             const char* message, const void* userParam) {
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
        case GL_DEBUG_TYPE_MARKER:
            type_str = "MARKER";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            type_str = "PUSH GROUP";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            type_str = "POP GROUP";
            break;
        case GL_DEBUG_TYPE_OTHER:
            type_str = "OTHER";
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
            SPONGE_GL_DEBUG("{} {} [{}]: {}", source_str, type_str, id,
                            message);
            break;
        default:
            SPONGE_GL_TRACE("{} {} [{}]: {}", source_str, type_str, id,
                            message);
            break;
    }
}
