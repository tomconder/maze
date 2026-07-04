#include "platform/opengl/scene/bitmapfont.hpp"

#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <string_view>

namespace {
constexpr size_t maxLength   = 256;
constexpr size_t indexCount  = 6;
constexpr size_t vertexCount = 8;

std::array<uint32_t, maxLength * indexCount>   batchIndices;
std::array<glm::vec2, maxLength * vertexCount> batchVertices;
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

BitmapFont::BitmapFont(const FontCreateInfo& createInfo) {
    assert(!createInfo.path.empty());

    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name               = shaderName.data(),
        .vertexShaderPath   = "/shaders/glsl/sprite.vert.glsl",
        .fragmentShaderPath = "/shaders/glsl/text.frag.glsl"
    };
    shader = AssetManager::createShader(shaderCreateInfo);
    shader->bind();

    vao = renderer::VertexArray::create();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(
        nullptr, maxLength * vertexCount * sizeof(glm::vec2));
    vbo->bind();

    ebo = std::make_unique<renderer::IndexBuffer>(
        nullptr, maxLength * indexCount * sizeof(uint32_t));
    ebo->bind();

    constexpr uint32_t pos = 0;
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                          nullptr);

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
        text.length() > maxLength ? text.substr(0, maxLength) : text;

    float penX = 0.0F;
    for (const auto& shapedGlyph : atlas.shape(str, size)) {
        penX += shapedGlyph.xAdvance;
    }

    return static_cast<uint32_t>(std::lround(penX));
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
        text.length() > maxLength ? text.substr(0, maxLength) : text;

    const float ascender   = atlas.getAscender(passTargetSize);
    float       penX       = position.x;
    uint32_t    glyphCount = 0;
    const auto  shaped     = atlas.shape(str, passTargetSize);

    for (const auto& shapedGlyph : shaped) {
        // quad sits on the whole pixel; the fractional remainder picks the
        // subpixel-shifted bitmap baked for that phase
        const float glyphX = penX + shapedGlyph.xOffset;
        const float xfloor = std::floor(glyphX);
        const auto  phase  = std::min(
            sponge::scene::FontAtlas::subpixelPhases - 1,
            static_cast<uint32_t>(
                (glyphX - xfloor) *
                static_cast<float>(sponge::scene::FontAtlas::subpixelPhases)));

        const sponge::scene::GlyphInfo* glyphInfo =
            atlas.getGlyph(shapedGlyph.codepoint, passTargetSize, phase);
        if (glyphInfo && glyphInfo->width > 0 && glyphInfo->height > 0) {
            const float xpos = xfloor + static_cast<float>(glyphInfo->bearingX);
            const float ypos =
                std::round(position.y - shapedGlyph.yOffset + ascender) -
                static_cast<float>(glyphInfo->bearingY);
            const float glyphWidth  = static_cast<float>(glyphInfo->width);
            const float glyphHeight = static_cast<float>(glyphInfo->height);

            const float uLeft   = glyphInfo->uvLeft;
            const float vTop    = glyphInfo->uvTop;
            const float uRight  = glyphInfo->uvLeft + glyphInfo->uvWidth;
            const float vBottom = glyphInfo->uvTop + glyphInfo->uvHeight;

            const std::array<glm::vec2, vertexCount> vertices{
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
                          static_cast<ptrdiff_t>(glyphCount * vertexCount));

            const std::array<uint32_t, indexCount> indices = {
                glyphCount * 4, (glyphCount * 4) + 2, (glyphCount * 4) + 1,
                glyphCount * 4, (glyphCount * 4) + 3, (glyphCount * 4) + 2
            };

            std::copy(indices.begin(), indices.end(),
                      batchIndices.begin() +
                          static_cast<ptrdiff_t>(glyphCount * indexCount));
            glyphCount++;
        }

        penX += shapedGlyph.xAdvance;
    }

    if (glyphCount == 0) {
        return;
    }

    shader->setFloat3("textColor", color);

    vbo->update(batchVertices.data(),
                glyphCount * vertexCount * sizeof(glm::vec2));
    ebo->update(batchIndices.data(), glyphCount * indexCount);

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(glyphCount * indexCount),
                   GL_UNSIGNED_INT, nullptr);
}

void BitmapFont::endPass() {
    passTargetSize = 0;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    shader->unbind();
    vao->unbind();
}

}  // namespace sponge::platform::opengl::scene
