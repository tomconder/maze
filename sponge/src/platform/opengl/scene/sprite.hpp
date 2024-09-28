#pragma once

#include "platform/opengl/renderer/indexbuffer.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"
#include "scene/sprite.hpp"
#include <memory>
#include <string>

namespace sponge::platform::opengl::scene {

class Sprite final : public sponge::scene::Sprite {
   public:
    explicit Sprite(const std::string& name, const std::string& texturePath);

    void render(const glm::vec2& position,
                const glm::vec2& size) const override;

    static std::string getShaderName() {
        return std::string(shaderName);
    };

   private:
    static constexpr char shaderName[] = "sprite";

    std::unique_ptr<renderer::VertexBuffer> vbo;
    std::unique_ptr<renderer::IndexBuffer> ebo;
    std::unique_ptr<renderer::VertexArray> vao;
    std::shared_ptr<renderer::Texture> tex;
};

}  // namespace sponge::platform::opengl::scene
