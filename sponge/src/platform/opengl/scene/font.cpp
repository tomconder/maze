#include "font.hpp"
#include "core/base.hpp"
#include "platform/opengl/renderer/resourcemanager.hpp"
#include <fmt/format.h>

namespace {
constexpr char vertex[] = "vertex";
}  // namespace

namespace sponge::platform::opengl::scene {

using renderer::ResourceManager;

Font::Font() {
    shader = ResourceManager::loadShader(shaderName, "/shaders/text.vert",
                                         "/shaders/text.frag");
    shader->bind();

    vao = renderer::VertexArray::create();
    vao->bind();

    vbo = renderer::VertexBuffer::create(maxLength * numVertices);
    vbo->bind();

    ebo = renderer::IndexBuffer::create(maxLength * numIndices);
    ebo->bind();

    const auto program = shader->getId();

    auto location = glGetAttribLocation(program, vertex);
    if (location != -1) {
        const auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE,
                              4 * sizeof(GLfloat), nullptr);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    shader->unbind();

    batchIndices.fill(0);
    batchVertices.fill(glm::vec2{ 0 });
}

void Font::load(const std::string& path) {
    sponge::scene::Font::load(path);

    const auto pos = path.find_last_of('/');
    const auto fontFolder = path.substr(0, pos + 1);

    const auto texture = ResourceManager::loadTexture(
        textureName, fontFolder + textureName, renderer::ExcludeAssetsFolder);
    UNUSED(texture);
}

void Font::render(const std::string& text, const glm::vec2& position,
                  uint32_t targetSize, const glm::vec3& color) {
    if (textureName.empty()) {
        // texture name is empty when the font fails to load
        return;
    }

    const auto fontSize = static_cast<float>(targetSize);
    const float scale = fontSize / size;
    const auto str =
        text.length() > maxLength ? text.substr(0, maxLength) : text;

    std::string prev;

    uint32_t x = position.x;

    for (uint32_t i = 0; i < str.size(); i++) {
        auto index = std::to_string(str[i]);
        auto [loc, width, height, offset, xadvance, page] = fontChars[index];

        const auto xpos = x + (offset.x * scale);
        const auto ypos = position.y + (offset.y * scale);

        const auto w = width * scale;
        const auto h = height * scale;

        const auto texx = loc.x / scaleW;
        const auto texy = loc.y / scaleH;
        const auto texh = height / scaleH;
        const auto texw = width / scaleW;

        const std::array<glm::vec2, numVertices> vertices{
            { { xpos, ypos + h },
              { texx, texy + texh },
              { xpos, ypos },
              { texx, texy },
              { xpos + w, ypos },
              { texx + texw, texy },
              { xpos + w, ypos + h },
              { texx + texw, texy + texh } }
        };

        std::move(vertices.begin(), vertices.end(),
                  batchVertices.begin() + (i * numVertices));

        const std::array indices = {
            i * 4, (i * 4) + 2, (i * 4) + 1,  //
            i * 4, (i * 4) + 3, (i * 4) + 2   //
        };

        std::move(indices.begin(), indices.end(),
                  batchIndices.begin() + (i * numIndices));

        x += xadvance * scale;

        if (!prev.empty()) {
            const auto key = fmt::format("{}.{}", prev, index);
            x += kerning[key] * scale;
        }
        prev = index;
    }

    vao->bind();

    shader->bind();
    shader->setFloat3("textColor", color);
    shader->setFloat("screenPxRange", fontSize / size * 4.0F);

    auto tex = ResourceManager::getTexture(textureName);
    tex->bind();

    uint32_t numChars = str.size();

    vbo->update(batchVertices.data(), numChars * numVertices);

    ebo->update(batchIndices.data(), numChars * numIndices);

    glDrawElements(GL_TRIANGLES, numChars * numIndices, GL_UNSIGNED_INT,
                   nullptr);
}

}  // namespace sponge::platform::opengl::scene