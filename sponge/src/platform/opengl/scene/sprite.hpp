#pragma once

#include "platform/opengl/renderer/indexbuffer.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <string>

namespace sponge::platform::opengl::scene {

class Sprite final {
public:
    explicit Sprite(const std::string& name, const std::string& texturePath);

    // Shares an already-loaded texture and draws one sub-rect of it. uvOffset
    // and uvScale are in normalized texture space. For atlas sprites.
    Sprite(std::shared_ptr<renderer::Texture> texture,
           const glm::vec2& uvOffset, const glm::vec2& uvScale);

    void render(const glm::vec2& position, const glm::vec2& size,
                std::optional<float> alpha) const;

    std::shared_ptr<renderer::Shader> getShader() const {
        return shader;
    }

private:
    static const std::string shaderName;

    void createBuffers();

    std::shared_ptr<renderer::Shader>  shader;
    std::shared_ptr<renderer::Texture> tex;

    glm::vec2 uvOffset{ 0.F, 0.F };
    glm::vec2 uvScale{ 1.F, 1.F };

    std::unique_ptr<renderer::IndexBuffer>  ebo;
    std::unique_ptr<renderer::VertexArray>  vao;
    std::unique_ptr<renderer::VertexBuffer> vbo;
};

}  // namespace sponge::platform::opengl::scene
