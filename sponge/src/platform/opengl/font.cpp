#include "font.hpp"
#include "core/base.hpp"
#include "platform/opengl/gl.hpp"
#include "platform/opengl/resourcemanager.hpp"
#include <vector>

namespace sponge::platform::opengl {

constexpr std::string_view textShader = "text";
constexpr std::string_view vertex = "vertex";

const std::vector<uint32_t> indices = {
    0, 1, 2,  //
    0, 2, 3   //
};

Font::Font() {
    ResourceManager::loadShader("/shaders/text.vert", "/shaders/text.frag",
                                textShader.data());

    auto shader = ResourceManager::getShader(textShader.data());
    shader->bind();

    const auto program = shader->getId();

    vao = std::make_unique<VertexArray>();
    vao->bind();

    vbo = std::make_unique<VertexBuffer>(maxLength * 8);
    vbo->bind();

    ebo = std::make_unique<IndexBuffer>(maxLength * 6);
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
    renderer::Font::load(path);

    auto pos = path.find_last_of('/');
    auto fontFolder = path.substr(0, pos + 1);

    auto texture = ResourceManager::loadTexture(
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
        auto ch = fontChars[index];

        const auto xpos = x + ch.offset.x * scale;
        const auto ypos = position.y + ch.offset.y * scale;

        const auto w = ch.width * scale;
        const auto h = ch.height * scale;

        const auto texx = ch.loc.x / scaleW;
        const auto texy = ch.loc.y / scaleH;
        const auto texh = ch.height / scaleH;
        const auto texw = ch.width / scaleW;

        const std::vector<glm::vec2> vertices = {
            { xpos, ypos + h },     { texx, texy + texh },        //
            { xpos, ypos },         { texx, texy },               //
            { xpos + w, ypos },     { texx + texw, texy },        //
            { xpos + w, ypos + h }, { texx + texw, texy + texh }  //
        };

        batchVertices.insert(batchVertices.end(), vertices.begin(),
                             vertices.end());

        const std::vector<uint32_t> indices = {
            numIndices, numIndices + 2, numIndices + 1,  //
            numIndices, numIndices + 3, numIndices + 2   //
        };

        batchIndices.insert(batchIndices.end(), indices.begin(), indices.end());

        x += ch.xadvance * scale;
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

}  // namespace sponge::platform::opengl
