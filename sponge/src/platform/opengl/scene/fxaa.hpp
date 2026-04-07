#pragma once

#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"

#include <cstdint>
#include <memory>
#include <string_view>

namespace sponge::platform::opengl::scene {

class FXAA {
public:
    FXAA() = delete;
    FXAA(uint32_t width, uint32_t height);
    ~FXAA();

    FXAA(const FXAA&)            = delete;
    FXAA& operator=(const FXAA&) = delete;

    void begin() const;
    void end() const;
    void apply() const;

    void resize(uint32_t newWidth, uint32_t newHeight);

    bool isEnabled() const {
        return enabled;
    }

    void setEnabled(const bool val) {
        enabled = val;
    }

    static std::string_view getShaderName() {
        return shaderName;
    }

private:
    static constexpr std::string_view shaderName = "fxaa";

    std::shared_ptr<renderer::Shader>       shader;
    std::unique_ptr<renderer::VertexArray>  vao;
    std::unique_ptr<renderer::VertexBuffer> vbo;

    // Raw GL handles — managed here because the Texture class has no
    // color-buffer creation path, and the framebuffer must be recreated on
    // every window resize.
    uint32_t colorTexture = 0;
    uint32_t depthRbo     = 0;
    uint32_t fbo          = 0;

    uint32_t width   = 0;
    uint32_t height  = 0;
    bool     enabled = true;

    void initialize();
    void createFramebuffer();
    void destroyFramebuffer();
};

}  // namespace sponge::platform::opengl::scene
