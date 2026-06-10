#pragma once

#include "platform/opengl/renderer/indexbuffer.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"
#include "platform/opengl/scene/msdffont.hpp"
#include "scene/fontatlas.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string_view>

namespace sponge::platform::opengl::scene {

class BitmapFont {
public:
    explicit BitmapFont(const FontCreateInfo& createInfo);
    ~BitmapFont();

    uint32_t getLength(std::string_view text, uint32_t size) const;
    uint32_t getHeight(uint32_t size) const;
    void     beginPass(uint32_t size);
    void     render(std::string_view text, const glm::vec2& position,
                    const glm::vec3& color);
    void     endPass();

    static std::string_view getShaderName() {
        return shaderName;
    }

private:
    static constexpr std::string_view shaderName = "text";

    uint32_t passTargetSize = 0;
    uint32_t textureId      = 0;

    sponge::scene::FontAtlas                atlas;
    std::shared_ptr<renderer::Shader>       shader;
    std::unique_ptr<renderer::VertexBuffer> vbo;
    std::unique_ptr<renderer::VertexArray>  vao;
    std::unique_ptr<renderer::IndexBuffer>  ebo;
};

}  // namespace sponge::platform::opengl::scene
