#include "font.hpp"
#include "core/base.hpp"
#include "platform/opengl/renderer/resourcemanager.hpp"
#include <fmt/format.h>
#include <vector>

namespace sponge::platform::opengl::renderer {

constexpr std::string_view textShader = "text";
constexpr std::string_view vertex = "vertex";

const std::vector<uint32_t> indices = {
    0, 1, 2,  //
    0, 2, 3   //
};

Font::Font() {
    ResourceManager::loadShader("/shaders/text.vert", "/shaders/text.frag",
                                textShader.data());

    const auto shader = ResourceManager::getShader(textShader.data());
    shader->bind();

    const auto program = shader->getId();

    vao = VertexArray::create();
    vao->bind();

    vbo = VertexBuffer::create(maxLength * 8);
    vbo->bind();

    ebo = IndexBuffer::create(maxLength * 6);
    ebo->bind();

    auto location = glGetAttribLocation(program, vertex.data());
    if (location != -1) {
        const auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE,
                              4 * sizeof(GLfloat), nullptr);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    shader->unbind();
}

void Font::load(const std::string& path) {
    sponge::renderer::Font::load(path);

    const auto pos = path.find_last_of('/');
    const auto fontFolder = path.substr(0, pos + 1);

    const auto texture = ResourceManager::loadTexture(
        fontFolder + textureName, textureName, ExcludeAssetsFolder);
    UNUSED(texture);
}

void Font::render(std::string_view text, const glm::vec2& position,
                  uint32_t targetSize, const glm::vec3& color) {
    if (textureName.empty()) {
        // texture name is empty when the font fails to load
        return;
    }

    const auto fontSize = static_cast<float>(targetSize);
    const float scale = fontSize / size;
    const auto str =
        text.length() > maxLength ? text.substr(0, maxLength) : text;

    std::vector<glm::vec2> batchVertices;
    std::vector<uint32_t> batchIndices;
    uint32_t numIndices = 0;
    std::string prev;

    uint32_t x = position.x;

    for (const char& c : str) {
        auto index = std::to_string(c);
        auto [loc, width, height, offset, xadvance, page] = fontChars[index];

        const auto xpos = x + offset.x * scale;
        const auto ypos = position.y + offset.y * scale;

        const auto w = width * scale;
        const auto h = height * scale;

        const auto texx = loc.x / scaleW;
        const auto texy = loc.y / scaleH;
        const auto texh = height / scaleH;
        const auto texw = width / scaleW;

        const std::vector<glm::vec2> vertices = {
            { xpos, ypos + h },     { texx, texy + texh },        //
            { xpos, ypos },         { texx, texy },               //
            { xpos + w, ypos },     { texx + texw, texy },        //
            { xpos + w, ypos + h }, { texx + texw, texy + texh }  //
        };

        batchVertices.insert(batchVertices.end(), vertices.begin(),
                             vertices.end());

        const std::vector indices = {
            numIndices, numIndices + 2, numIndices + 1,  //
            numIndices, numIndices + 3, numIndices + 2   //
        };

        batchIndices.insert(batchIndices.end(), indices.begin(), indices.end());

        x += xadvance * scale;
        numIndices += 4;

        if (!prev.empty()) {
            const auto key = fmt::format("{}.{}", prev, index);
            x += kerning[key] * scale;
        }
        prev = index;
    }

    auto shader = ResourceManager::getShader(textShader.data());

    vao->bind();

    shader->bind();
    shader->setFloat3("textColor", color);
    shader->setFloat("screenPxRange", fontSize / size * 4.0F);

    auto tex = ResourceManager::getTexture(textureName);
    tex->bind();

    vbo->update(batchVertices);

    ebo->update(batchIndices);

    glDrawElements(GL_TRIANGLES, static_cast<int32_t>(batchIndices.size()),
                   GL_UNSIGNED_INT, nullptr);
}

}  // namespace sponge::platform::opengl::renderer
