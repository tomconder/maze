#pragma once

#include "platform/opengl/renderer/indexbuffer.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"
#include "renderer/font.hpp"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <string>

namespace sponge::platform::opengl::renderer {

class Font : public sponge::renderer::Font {
   public:
    Font();
    void load(const std::string& path);
    void render(std::string_view text, const glm::vec2& position,
                uint32_t targetSize, const glm::vec3& color);

   private:
    std::unique_ptr<VertexBuffer> vbo;
    std::unique_ptr<VertexArray> vao;
    std::unique_ptr<IndexBuffer> ebo;
};

}  // namespace sponge::platform::opengl::renderer