#pragma once

struct GLFWwindow;

namespace sponge::platform::opengl::renderer {

class Context final {
public:
    Context();
    void init(GLFWwindow* window);

    void flip(void* window);
};

}  // namespace sponge::platform::opengl::renderer
