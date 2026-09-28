#include "platform/opengl/renderer/context.hpp"

#include "logging/log.hpp"
#include "platform/opengl/debug/profiler.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <GLFW/glfw3.h>
#include <fmt/format.h>

#include <cstdint>
#include <cstdlib>
#include <string>

namespace {
constexpr int minGLMajor = 4;
constexpr int minGLMinor = 5;
}  // namespace

namespace sponge::platform::opengl::renderer {
Context::Context() {
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // dpi scaling
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, 1);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, minGLMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minGLMinor);

    // Hidden until the application has finished onUserCreate().
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

#ifdef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
}

void Context::init(GLFWwindow* window) const {
    SPONGE_GL_INFO("Initializing context");

    glfwMakeContextCurrent(window);
    if (glfwGetCurrentContext() == nullptr) {
        const char* descCStr = nullptr;
        glfwGetError(&descCStr);
        const std::string description = descCStr ? descCStr : "Unknown error";
        SPONGE_GL_ERROR("Context could not be created: {}", description);
        return;
    }

    const int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        SPONGE_GL_ERROR("Failed to initialize OpenGL context");
        return;
    }

    const int major = GLAD_VERSION_MAJOR(version);
    const int minor = GLAD_VERSION_MINOR(version);
    if (major < minGLMajor || (major == minGLMajor && minor < minGLMinor)) {
        SPONGE_GL_CRITICAL("OpenGL {}.{} or later is required (found {}.{})",
                           minGLMajor, minGLMinor, major, minor);
        fmt::print(stderr,
                   "Error: OpenGL {}.{} or later is required (found {}.{})\n",
                   minGLMajor, minGLMinor, major, minor);
        std::exit(EXIT_FAILURE);
    }

    if (window != nullptr) {
        int32_t width  = 0;
        int32_t height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
    }
}

void Context::release(void* window) const {
    UNUSED(window);
    glfwMakeContextCurrent(nullptr);
}

void Context::makeCurrent(void* window) const {
    glfwMakeContextCurrent(static_cast<GLFWwindow*>(window));
}

void Context::flip(void* window) const {
    if (window == nullptr) {
        return;
    }
    glfwSwapBuffers(static_cast<GLFWwindow*>(window));
    SPONGE_PROFILE_GPU_COLLECT;
}
}  // namespace sponge::platform::opengl::renderer
