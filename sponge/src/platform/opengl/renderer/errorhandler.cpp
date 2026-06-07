#include "platform/opengl/renderer/errorhandler.hpp"

#include "core/base.hpp"
#include "glad/gl.h"
#include "logging/log.hpp"

#include <string>

void GLAPIENTRY glLogMessage(GLenum source, GLenum type, uint32_t id,
                             GLenum severity, GLsizei length,
                             const char* message, const void* userParam) {
    std::string sourceStr;
    std::string typeStr;

    UNUSED(id);
    UNUSED(length);
    UNUSED(message);
    UNUSED(userParam);

    switch (source) {
        case GL_DEBUG_SOURCE_API:
            sourceStr = "API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            sourceStr = "WINDOW SYSTEM";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            sourceStr = "SHADER COMPILER";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            sourceStr = "THIRD PARTY";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            sourceStr = "APPLICATION";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            sourceStr = "OTHER";
            break;
        default:
            sourceStr = "UNKNOWN";
            break;
    }

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            typeStr = "ERROR";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            typeStr = "DEPRECATED BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            typeStr = "UNDEFINED BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            typeStr = "PORTABILITY";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            typeStr = "PERFORMANCE";
            break;
        case GL_DEBUG_TYPE_MARKER:
            typeStr = "MARKER";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            typeStr = "PUSH GROUP";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            typeStr = "POP GROUP";
            break;
        case GL_DEBUG_TYPE_OTHER:
            typeStr = "OTHER";
            break;
        default:
            typeStr = "UNKNOWN";
            break;
    }

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            SPONGE_GL_ERROR("{} {} [{}]: {}", sourceStr, typeStr, id, message);
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            SPONGE_GL_WARN("{} {} [{}]: {}", sourceStr, typeStr, id, message);
            break;
        case GL_DEBUG_SEVERITY_LOW:
            SPONGE_GL_INFO("{} {} [{}]: {}", sourceStr, typeStr, id, message);
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            SPONGE_GL_DEBUG("{} {} [{}]: {}", sourceStr, typeStr, id, message);
            break;
        default:
            SPONGE_GL_TRACE("{} {} [{}]: {}", sourceStr, typeStr, id, message);
            break;
    }
}
