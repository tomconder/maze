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
    atlas.build({ { ttfPath } }, { 16, 48 });

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8,
                 static_cast<GLsizei>(atlas.atlasWidth()),
                 static_cast<GLsizei>(atlas.atlasHeight()), 0, GL_RED,
                 GL_UNSIGNED_BYTE, atlas.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
                               const uint32_t         size) const {
    const auto str =
        text.length() > kMaxLength ? text.substr(0, kMaxLength) : text;

    float    x    = 0.0F;
    char32_t prev = 0;

    for (const char ch : str) {
        const auto  c  = static_cast<char32_t>(ch);
        const auto* gi = atlas.getGlyph(c, size);
        if (!gi) {
            continue;
        }
        if (prev != 0) {
            x += atlas.getKerning(prev, c, size);
        }
        x += gi->advanceX;
        prev = c;
    }

    return static_cast<uint32_t>(x);
}

void BitmapFont::beginPass(const uint32_t size) {
    assert(textureId != 0);
    passTargetSize = size;
    vao->bind();
    shader->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
}

void BitmapFont::render(const std::string_view text, const glm::vec2& position,
                        const glm::vec3& color) {
    assert(passTargetSize != 0);

    const auto str =
        text.length() > kMaxLength ? text.substr(0, kMaxLength) : text;

    const float ascender = atlas.getAscender(passTargetSize);
    float       x        = position.x;
    char32_t    prev     = 0;
    uint32_t    i        = 0;

    for (const char ch : str) {
        const auto  c  = static_cast<char32_t>(ch);
        const auto* gi = atlas.getGlyph(c, passTargetSize);
        if (!gi) {
            continue;
        }

        if (prev != 0) {
            x += atlas.getKerning(prev, c, passTargetSize);
        }

        if (gi->width > 0 && gi->height > 0) {
            const float xpos = x + static_cast<float>(gi->bearingX);
            const float ypos =
                position.y + ascender - static_cast<float>(gi->bearingY);
            const float w = static_cast<float>(gi->width);
            const float h = static_cast<float>(gi->height);

            const float u0 = gi->u;
            const float v0 = gi->v;
            const float u1 = gi->u + gi->uvW;
            const float v1 = gi->v + gi->uvH;

            const std::array<glm::vec2, kVertexCount> vertices{
                { { xpos, ypos + h },
                  { u0, v1 },
                  { xpos, ypos },
                  { u0, v0 },
                  { xpos + w, ypos },
                  { u1, v0 },
                  { xpos + w, ypos + h },
                  { u1, v1 } }
            };

            std::copy(vertices.begin(), vertices.end(),
                      batchVertices.begin() +
                          static_cast<ptrdiff_t>(i * kVertexCount));

            const std::array<uint32_t, kIndexCount> indices = {
                i * 4, (i * 4) + 2, (i * 4) + 1, i * 4, (i * 4) + 3, (i * 4) + 2
            };

            std::copy(indices.begin(), indices.end(),
                      batchIndices.begin() +
                          static_cast<ptrdiff_t>(i * kIndexCount));
            ++i;
        }

        x += gi->advanceX;
        prev = c;
    }

    if (i == 0) {
        return;
    }

    shader->setFloat3("textColor", color);

    vbo->update(batchVertices.data(), i * kVertexCount * sizeof(glm::vec2));
    ebo->update(batchIndices.data(), i * kIndexCount);

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(i * kIndexCount),
                   GL_UNSIGNED_INT, nullptr);
}

void BitmapFont::endPass() {
    passTargetSize = 0;
    shader->unbind();
    vao->unbind();
}

}  // namespace sponge::platform::opengl::scene
