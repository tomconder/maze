#include "platform/opengl/scene/bitmapfont.hpp"

#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"
#include "platform/opengl/renderer/texture.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <memory>
#include <string_view>

namespace {
constexpr size_t maxLength   = 256;
constexpr size_t indexCount  = 6;
constexpr size_t vertexCount = 8;

std::array<glm::vec2, maxLength * vertexCount> batchVertices;

// quad index pattern is fixed, so the index buffer is filled once at startup
std::array<uint32_t, maxLength * indexCount> makeQuadIndices() {
    std::array<uint32_t, maxLength * indexCount> indices{};
    for (uint32_t quad = 0; quad < maxLength; quad++) {
        const uint32_t base            = quad * 4;
        indices[quad * indexCount]     = base;
        indices[quad * indexCount + 1] = base + 2;
        indices[quad * indexCount + 2] = base + 1;
        indices[quad * indexCount + 3] = base;
        indices[quad * indexCount + 4] = base + 3;
        indices[quad * indexCount + 5] = base + 2;
    }
    return indices;
}
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

BitmapFont::BitmapFont(const FontCreateInfo& createInfo) {
    assert(!createInfo.path.empty());

    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name               = shaderName.data(),
        .vertexShaderPath   = "/shaders/glsl/sprite.vert.glsl",
        .fragmentShaderPath = "/shaders/glsl/text.frag.glsl",
    };
    shader = AssetManager::createShader(shaderCreateInfo);
    shader->bind();

    vao = std::make_unique<renderer::VertexArray>();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(
        nullptr, maxLength * vertexCount * sizeof(glm::vec2));
    vbo->bind();

    const auto quadIndices = makeQuadIndices();
    ebo                    = std::make_unique<renderer::IndexBuffer>(
        quadIndices.data(), quadIndices.size() * sizeof(uint32_t));
    ebo->bind();

    constexpr uint32_t pos = 0;
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                          nullptr);

    vbo->unbind();
    vao->unbind();
    shader->unbind();

    const std::string ttfPath = createInfo.assetsFolder + createInfo.path;
    atlas.build(ttfPath, { 18, 24, 32, 48 });

    // The atlas holds per-channel LCD coverage, not colour, so no gamma
    // correction: an sRGB internal format would apply a transfer function to
    // the coverage that feeds the dual-source blend in beginPass().
    const renderer::TextureCreateInfo textureCreateInfo{
        .name          = createInfo.name,
        .path          = "",
        .width         = atlas.atlasWidth(),
        .height        = atlas.atlasHeight(),
        .bytesPerPixel = 3,
        .data          = atlas.data(),
        .loadFlag      = renderer::Pixelated,
    };
    texture = std::make_unique<renderer::Texture>(textureCreateInfo);
}

uint32_t BitmapFont::getHeight(const uint32_t size) const {
    return static_cast<uint32_t>(atlas.getLineHeight(size));
}

uint32_t BitmapFont::getLength(const std::string_view text, const uint32_t size,
                               const bool tabularFigures) {
    const auto str =
        text.length() > maxLength ? text.substr(0, maxLength) : text;

    float penX = 0.0F;
    for (const auto& shapedGlyph : atlas.shape(str, size, tabularFigures)) {
        penX += shapedGlyph.xAdvance;
    }

    return static_cast<uint32_t>(std::lround(penX));
}

void BitmapFont::beginPass(const uint32_t size) {
    assert(texture);
    passTargetSize = size;
    vao->bind();
    shader->bind();
    texture->activateAndBind(0);
    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
}

void BitmapFont::render(const std::string_view text, const glm::vec2& position,
                        const glm::vec3& color, const bool tabularFigures) {
    assert(passTargetSize != 0);

    const auto str =
        text.length() > maxLength ? text.substr(0, maxLength) : text;

    const float ascender   = atlas.getAscender(passTargetSize);
    float       penX       = position.x;
    uint32_t    glyphCount = 0;
    const auto  shaped     = atlas.shape(str, passTargetSize, tabularFigures);

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
            atlas.getGlyph(shapedGlyph.glyphIndex, passTargetSize, phase);
        if (glyphInfo && glyphInfo->width > 0 && glyphInfo->height > 0) {
            const float xpos = xfloor + static_cast<float>(glyphInfo->bearingX);
            const float ypos =
                std::round(position.y - shapedGlyph.yOffset + ascender) -
                static_cast<float>(glyphInfo->bearingY);
            const auto glyphWidth  = static_cast<float>(glyphInfo->width);
            const auto glyphHeight = static_cast<float>(glyphInfo->height);

            const float uLeft   = glyphInfo->uvLeft;
            const float vTop    = glyphInfo->uvTop;
            const float uRight  = glyphInfo->uvLeft + glyphInfo->uvWidth;
            const float vBottom = glyphInfo->uvTop + glyphInfo->uvHeight;

            const std::array<glm::vec2, vertexCount> vertices{
                {
                    { xpos, ypos + glyphHeight },
                    { uLeft, vBottom },
                    { xpos, ypos },
                    { uLeft, vTop },
                    { xpos + glyphWidth, ypos },
                    { uRight, vTop },
                    { xpos + glyphWidth, ypos + glyphHeight },
                    { uRight, vBottom },
                },
            };

            std::ranges::copy(
                vertices, batchVertices.begin() +
                              static_cast<ptrdiff_t>(glyphCount * vertexCount));
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
