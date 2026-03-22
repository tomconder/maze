#pragma once

struct GLFWwindow;

namespace sponge::platform::opengl::renderer {

class Context final {
public:
    Context();
    void init(GLFWwindow* window);

    // Release GL context from the calling thread; call before starting the
    // render thread.
    static void release(void* window);

    // Make GL context current on the calling thread.
    static void makeCurrent(void* window);

    void flip(void* window);
};

}  // namespace sponge::platform::opengl::renderer
