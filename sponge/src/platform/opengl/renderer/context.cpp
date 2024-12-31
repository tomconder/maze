#include "context.hpp"
#include "logging/log.hpp"
#include "platform/opengl/debug/profiler.hpp"
#include "platform/opengl/renderer/gl.hpp"

namespace sponge::platform::opengl::renderer {

Context::Context() {
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
#endif

#ifdef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
}

void Context::init(GLFWwindow* window) {
    SPONGE_CORE_INFO("Initializing OpenGL");

    glfwMakeContextCurrent(window);
    if (glfwGetCurrentContext() == nullptr) {
        const char* description;
        glfwGetError(&description);
        SPONGE_CORE_ERROR("OpenGL context could not be created: {}",
                          description);
        return;
    }

    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));

    SPONGE_PROFILE_GPU_CONTEXT;

    if (window != nullptr) {
        int32_t width;
        int32_t height;
        glfwGetWindowSize(window, &width, &height);
        glViewport(0, 0, width, height);
    }
}

void Context::flip(void* window) {
    if (window == nullptr) {
        return;
    }
    glfwSwapBuffers(static_cast<GLFWwindow*>(window));
    SPONGE_PROFILE_GPU_COLLECT;
}

}  // namespace sponge::platform::opengl::renderer
