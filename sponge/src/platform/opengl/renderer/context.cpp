#include "context.hpp"
#include "logging/log.hpp"
#include "platform/opengl/debug/profiler.hpp"
#include "platform/opengl/renderer/gl.hpp"

namespace {
constexpr std::pair<int, int> glVersions[13] = {
    { 4, 6 }, { 4, 5 }, { 4, 4 }, { 4, 3 }, { 4, 2 }, { 4, 1 }, { 4, 0 },
    { 3, 3 }, { 3, 2 }, { 3, 1 }, { 3, 0 }, { 2, 1 }, { 2, 0 }
};
}

namespace sponge::platform::opengl::renderer {

Context::Context(GLFWwindow* window) {
    SPONGE_CORE_INFO("Initializing OpenGL");

    glfwWindowHint(GLFW_SAMPLES, 8);

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    SPONGE_CORE_INFO("Creating OpenGL context");

    // create context trying different versions
    for (const auto& [major, minor] : glVersions) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
        glfwMakeContextCurrent(window);
        if (glfwGetCurrentContext() != nullptr) {
            break;
        }
    }

    if (glfwGetCurrentContext() == nullptr) {
        const char* description;
        glfwGetError(&description);
        SPONGE_CORE_ERROR("OpenGL context could not be created: {}",
                          description);
        return;
    }

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

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
