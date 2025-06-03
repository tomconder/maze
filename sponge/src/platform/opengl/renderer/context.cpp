#include "context.hpp"
#include "info.hpp"
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
Context::Context() {
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // dpi scaling
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, 1);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#else
    // create window trying different versions
    for (const auto& [major, minor] : glVersions) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        GLFWwindow* window =
            glfwCreateWindow(640, 480, "GL Version Test", nullptr, nullptr);
        if (window) {
            glfwDestroyWindow(window);
            break;
        }
    }
#endif

#ifdef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
}

void Context::init(GLFWwindow* window) {
    SPONGE_CORE_INFO("Initializing context");

    glfwMakeContextCurrent(window);
    if (glfwGetCurrentContext() == nullptr) {
        const char* description = nullptr;
        glfwGetError(&description);
        SPONGE_CORE_ERROR("Context could not be created: {}", description);
        return;
    }

    const int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        SPONGE_CORE_ERROR("Failed to initialize OpenGL context");
        return;
    }

    Info::logInfo();

    SPONGE_PROFILE_GPU_CONTEXT;

    if (window != nullptr) {
        float xscale = 0.F;
        float yscale = 0.F;
        glfwGetWindowContentScale(window, &xscale, &yscale);

        int32_t width = 0;
        int32_t height = 0;
        glfwGetFramebufferSize(window, &width, &height);
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
