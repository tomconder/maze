#include "renderer/opengl/openglmesh.h"
#include "renderer/opengl/openglresourcemanager.h"
#include <cstddef>

namespace sponge {

OpenGLMesh::OpenGLMesh(
    const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
    const std::vector<std::shared_ptr<OpenGLTexture>>& textures)
    : textures(textures) {
    this->indices = indices;
    this->vertices = vertices;

    vao = std::make_unique<OpenGLVertexArray>();
    vao->bind();

    vbo = std::make_unique<OpenGLBuffer>(
        reinterpret_cast<const float*>(vertices.data()),
        static_cast<uint32_t>(vertices.size()) *
            static_cast<uint32_t>(sizeof(Vertex)));
    vbo->bind();

    ebo = std::make_unique<OpenGLElementBuffer>(
        indices.data(), static_cast<uint32_t>(indices.size()) *
                            static_cast<uint32_t>(sizeof(unsigned int)));
    ebo->bind();

    auto shader = OpenGLResourceManager::getShader("shader");
    shader->bind();

    uint32_t program = shader->getId();

    auto location = glGetAttribLocation(program, "position");
    if (location != -1) {
        auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, position));
    }

    location = glGetAttribLocation(program, "normal");
    if (location != -1) {
        auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, normal));
    }

    location = glGetAttribLocation(program, "texCoord");
    if (location != -1) {
        auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, texCoords));
    }

    location = glGetAttribLocation(program, "tangent");
    if (location != -1) {
        auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, tangent));
    }

    location = glGetAttribLocation(program, "biTangent");
    if (location != -1) {
        auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, biTangent));
    }

    shader->unbind();
    glBindVertexArray(0);
}

void OpenGLMesh::render() const {
    vao->bind();

    std::shared_ptr<OpenGLShader> shader =
        OpenGLResourceManager::getShader("shader");
    shader->bind();

    if (!textures.empty()) {
        shader->setInteger("texture_diffuse1", 0);
        textures[0]->bind();
    } else {
        shader->setBoolean("hasNoTexture", true);
    }

    glDrawElements(GL_TRIANGLES, static_cast<GLint>(indices.size()),
                   GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

}  // namespace sponge
