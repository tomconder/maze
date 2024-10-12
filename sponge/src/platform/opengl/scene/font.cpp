#include "font.hpp"
#include "core/base.hpp"
#include "platform/opengl/renderer/resourcemanager.hpp"
#include <fmt/format.h>
#include <array>

namespace {
constexpr char vertex[] = "vertex";
constexpr size_t indexCount = 6;
constexpr size_t vertexCount = 8;
constexpr size_t maxLength = 256;

std::array<uint32_t, maxLength * indexCount> batchIndices;
std::array<glm::vec2, maxLength * vertexCount> batchVertices;
}  // namespace

namespace sponge::platform::opengl::scene {

using renderer::ResourceManager;

Font::Font() {
    shader = ResourceManager::loadShader(shaderName, "/shaders/text.vert.glsl",
                                         "/shaders/text.frag.glsl");
    shader->bind();

    vao = renderer::VertexArray::create();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(
        nullptr, maxLength * vertexCount * sizeof(glm::vec2));
    vbo->bind();

    ebo = std::make_unique<renderer::IndexBuffer>(
        nullptr, maxLength * indexCount * sizeof(uint32_t));
    ebo->bind();

    const auto program = shader->getId();

    if (const auto location = glGetAttribLocation(program, vertex);
        location != -1) {
        const auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE,
                              4 * sizeof(GLfloat), nullptr);
    }

    vbo->unbind();
    vao->unbind();

    shader->unbind();
}

uint32_t Font::getLength(const std::string_view text,
                         const uint32_t targetSize) {
    const auto scale = static_cast<float>(targetSize) / size;
    const auto str =
        text.length() > maxLength ? text.substr(0, maxLength) : text;

    std::string prev;
    uint32_t x = 0;

    for (const char& c : str) {
        auto index = std::to_string(c);
        auto [loc, width, height, offset, xadvance, page] = fontChars[index];
        x += xadvance * scale;
        if (!prev.empty()) {
            const auto key = fmt::format("{}.{}", prev, index);
            x += kerning[key] * scale;
        }
        prev = index;
    }

    return x;
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
                  const uint32_t targetSize, const glm::vec3& color) {
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

        const std::array<glm::vec2, vertexCount> vertices{
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
                  batchVertices.begin() + (i * vertexCount));

        const std::array indices = {
            i * 4, (i * 4) + 2, (i * 4) + 1,  //
            i * 4, (i * 4) + 3, (i * 4) + 2   //
        };

        std::move(indices.begin(), indices.end(),
                  batchIndices.begin() + (i * indexCount));

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

    const auto tex = ResourceManager::getTexture(textureName);
    tex->bind();

    const uint32_t numChars = str.size();

    vbo->update(batchVertices.data(),
                numChars * vertexCount * sizeof(glm::vec2));

    ebo->update(batchIndices.data(), numChars * indexCount);

    glDrawElements(GL_TRIANGLES, numChars * indexCount, GL_UNSIGNED_INT,
                   nullptr);

    shader->unbind();

    vao->unbind();
}

}  // namespace sponge::platform::opengl::scene
