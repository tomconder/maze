#include "platform/opengl/scene/bitmapfont.hpp"

#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <array>
#include <cassert>
#include <string_view>

namespace {
inline constexpr std::string_view vertex       = "vertex";
constexpr size_t                  kMaxLength   = 256;
constexpr size_t                  kIndexCount  = 6;
constexpr size_t                  kVertexCount = 8;

std::array<uint32_t, kMaxLength * kIndexCount>   batchIndices;
std::array<glm::vec2, kMaxLength * kVertexCount> batchVertices;
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

BitmapFont::BitmapFont(const FontCreateInfo& createInfo) {
    assert(!createInfo.path.empty());

    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name               = shaderName.data(),
        .vertexShaderPath   = "/shaders/glsl/text.vert.glsl",
        .fragmentShaderPath = "/shaders/glsl/text.frag.glsl"
    };
    shader = AssetManager::createShader(shaderCreateInfo);
    shader->bind();

    vao = renderer::VertexArray::create();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(
        nullptr, kMaxLength * kVertexCount * sizeof(glm::vec2));
    vbo->bind();

    ebo = std::make_unique<renderer::IndexBuffer>(
        nullptr, kMaxLength * kIndexCount * sizeof(uint32_t));
    ebo->bind();

    const auto program = shader->getId();
    if (const auto location = glGetAttribLocation(program, vertex.data());
        location != -1) {
        const auto pos = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(pos);
        glVertexAttribPointer(pos, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                              nullptr);
    }

    vbo->unbind();
    vao->unbind();
    shader->unbind();

    const std::string ttfPath = createInfo.assetsFolder + createInfo.path;
    atlas.build({ { ttfPath } }, { 16, 24, 32, 48 });

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                 static_cast<GLsizei>(atlas.atlasWidth()),
                 static_cast<GLsizei>(atlas.atlasHeight()), 0, GL_RGB,
                 GL_UNSIGNED_BYTE, atlas.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

BitmapFont::~BitmapFont() {
    if (textureId != 0) {
        glDeleteTextures(1, &textureId);
    }
}

uint32_t BitmapFont::getHeight(const uint32_t size) const {
    return static_cast<uint32_t>(atlas.getLineHeight(size));
}

uint32_t BitmapFont::getLength(const std::string_view text,
                               const uint32_t         size) {
    const auto str =
        text.length() > kMaxLength ? text.substr(0, kMaxLength) : text;

    float penX = 0.0F;
    for (const auto& shapedGlyph : atlas.shape(str, size)) {
        penX += shapedGlyph.xAdvance;
    }

    return static_cast<uint32_t>(penX);
}

void BitmapFont::beginPass(const uint32_t size) {
    assert(textureId != 0);
    passTargetSize = size;
    vao->bind();
    shader->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
}

void BitmapFont::render(const std::string_view text, const glm::vec2& position,
                        const glm::vec3& color) {
    assert(passTargetSize != 0);

    const auto str =
        text.length() > kMaxLength ? text.substr(0, kMaxLength) : text;

    const float ascender   = atlas.getAscender(passTargetSize);
    float       penX       = position.x;
    uint32_t    glyphCount = 0;
    const auto  shaped     = atlas.shape(str, passTargetSize);

    for (const auto& shapedGlyph : shaped) {
        const sponge::scene::GlyphInfo* glyphInfo = shapedGlyph.glyphInfo;
        if (glyphInfo && glyphInfo->width > 0 && glyphInfo->height > 0) {
            const float xpos = penX + shapedGlyph.xOffset +
                               static_cast<float>(glyphInfo->bearingX);
            const float ypos = position.y - shapedGlyph.yOffset + ascender -
                               static_cast<float>(glyphInfo->bearingY);
            const float glyphWidth  = static_cast<float>(glyphInfo->width);
            const float glyphHeight = static_cast<float>(glyphInfo->height);

            const float uLeft   = glyphInfo->uvLeft;
            const float vTop    = glyphInfo->uvTop;
            const float uRight  = glyphInfo->uvLeft + glyphInfo->uvWidth;
            const float vBottom = glyphInfo->uvTop + glyphInfo->uvHeight;

            const std::array<glm::vec2, kVertexCount> vertices{
                { { xpos, ypos + glyphHeight },
                  { uLeft, vBottom },
                  { xpos, ypos },
                  { uLeft, vTop },
                  { xpos + glyphWidth, ypos },
                  { uRight, vTop },
                  { xpos + glyphWidth, ypos + glyphHeight },
                  { uRight, vBottom } }
            };

            std::copy(vertices.begin(), vertices.end(),
                      batchVertices.begin() +
                          static_cast<ptrdiff_t>(glyphCount * kVertexCount));

            const std::array<uint32_t, kIndexCount> indices = {
                glyphCount * 4, (glyphCount * 4) + 2, (glyphCount * 4) + 1,
                glyphCount * 4, (glyphCount * 4) + 3, (glyphCount * 4) + 2
            };

            std::copy(indices.begin(), indices.end(),
                      batchIndices.begin() +
                          static_cast<ptrdiff_t>(glyphCount * kIndexCount));
            glyphCount++;
        }

        penX += shapedGlyph.xAdvance;
    }

    if (glyphCount == 0) {
        return;
    }

    shader->setFloat3("textColor", color);

    vbo->update(batchVertices.data(),
                glyphCount * kVertexCount * sizeof(glm::vec2));
    ebo->update(batchIndices.data(), glyphCount * kIndexCount);

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(glyphCount * kIndexCount),
                   GL_UNSIGNED_INT, nullptr);
}

void BitmapFont::endPass() {
    passTargetSize = 0;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    shader->unbind();
    vao->unbind();
}

}  // namespace sponge::platform::opengl::scene
