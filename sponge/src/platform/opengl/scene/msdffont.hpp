#pragma once

#include "platform/opengl/renderer/indexbuffer.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"
#include "scene/font.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace sponge::platform::opengl::scene {
struct FontCreateInfo {
    std::string name;
    std::string path;
    std::string assetsFolder = core::File::getResourceDir();
};

class MSDFFont : public sponge::scene::Font {
public:
    explicit MSDFFont(const FontCreateInfo& createInfo);
    uint32_t getLength(std::string_view text, uint32_t targetSize);
    void     render(const std::string& text, const glm::vec2& position,
                    uint32_t targetSize, const glm::vec3& color);

    static std::string_view getShaderName() {
        return shaderName;
    }

private:
    static constexpr std::string_view shaderName = "text";

    std::shared_ptr<renderer::Shader> shader;

    std::unique_ptr<renderer::VertexBuffer> vbo;
    std::unique_ptr<renderer::VertexArray>  vao;
    std::unique_ptr<renderer::IndexBuffer>  ebo;

    void load(const std::string& path);
};

}  // namespace sponge::platform::opengl::scene
