#pragma once

struct GLFWwindow;

namespace sponge::platform::opengl::renderer {

class Context final {
public:
    Context();
    void init(GLFWwindow* window) const;

    // Release GL context from the calling thread; call before starting the
    // render thread.
    void release(void* window) const;

    // Make GL context current on the calling thread.
    void makeCurrent(void* window) const;

    void flip(void* window) const;
};

}  // namespace sponge::platform::opengl::renderer
