#pragma once

#include "platform/opengl/renderer/indexbuffer.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"
#include "scene/font.hpp"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <array>
#include <string>

namespace sponge::platform::opengl::scene {
class Font : public sponge::scene::Font {
   public:
    Font();
    void load(const std::string& path);
    void render(const std::string& text, const glm::vec2& position,
                uint32_t targetSize, const glm::vec3& color);

    static std::string getShaderName() {
        return std::string(shaderName);
    };

   private:
    static constexpr char shaderName[] = "text";

    static constexpr size_t numIndices = 6;
    static constexpr size_t numVertices = 8;

    std::array<uint32_t, maxLength * numIndices> batchIndices;
    std::array<glm::vec2, maxLength * numVertices> batchVertices;

    std::shared_ptr<renderer::Shader> shader;

    std::unique_ptr<renderer::VertexBuffer> vbo;
    std::unique_ptr<renderer::VertexArray> vao;
    std::unique_ptr<renderer::IndexBuffer> ebo;
};

}  // namespace sponge::platform::opengl::scene