#include "platform/opengl/openglfont.h"
#include "platform/opengl/openglresourcemanager.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <sstream>

namespace sponge::graphics::renderer {

OpenGLFont::OpenGLFont() {
    auto shader = OpenGLResourceManager::getShader("text");
    shader->bind();

    vao = std::make_unique<OpenGLVertexArray>();
    vao->bind();

    vbo = std::make_unique<OpenGLBuffer>(
        maxLength * static_cast<uint32_t>(sizeof(float)) * 16);
    vbo->bind();

    ebo = std::make_unique<OpenGLElementBuffer>(
        maxLength * static_cast<uint32_t>(sizeof(uint32_t)) * 6);
    ebo->bind();

    auto program = shader->getId();
    auto position =
        static_cast<uint32_t>(glGetAttribLocation(program, "vertex"));
    glEnableVertexAttribArray(position);
    glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                          nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    shader->unbind();
}

void OpenGLFont::load(const std::string& path) {
    assert(!path.empty());

    std::ifstream stream(path, std::ios::in | std::ios::binary);
    assert(stream.good());

    auto nextInt = [](std::stringstream& sstream) {
        std::string s;
        sstream >> s;
        if (size_t pos = s.find_last_of('='); pos != std::string::npos) {
            return std::stoi(s.substr(pos + 1));
        }
        return 0;
    };

    auto nextFloat = [](std::stringstream& sstream) {
        std::string s;
        sstream >> s;
        if (size_t pos = s.find_last_of('='); pos != std::string::npos) {
            return std::stof(s.substr(pos + 1));
        }
        return 0.F;
    };

    auto nextString = [](std::stringstream& sstream) {
        std::string s;
        sstream >> s;
        if (size_t pos = s.find_last_of('='); pos != std::string::npos) {
            auto str = s.substr(pos + 1);
            // remove the surrounding quotes
            str.erase(str.begin());
            str.erase(str.end() - 1);
            return str;
        }
        return std::string{};
    };

    while (!stream.eof()) {
        std::string line;
        std::stringstream lineStream;
        std::getline(stream, line);
        lineStream << line;

        std::string str;
        lineStream >> str;

        if (str == "info") {
            face = nextString(lineStream);
            size = nextFloat(lineStream);
        }

        if (str == "common") {
            lineHeight = nextFloat(lineStream);
            base = nextFloat(lineStream);
            scaleW = nextFloat(lineStream);
            scaleH = nextFloat(lineStream);
            pages = nextInt(lineStream);
        }

        if (str == "page") {
            uint32_t id = nextInt(lineStream);
            UNUSED(id);
            std::string name = nextString(lineStream);

            auto pos = path.find_last_of('/');
            auto fontFolder = path.substr(0, pos + 1);
            textureName = name;
            auto texture = OpenGLResourceManager::loadTexture(fontFolder + name,
                                                              textureName);
        }

        if (str == "char") {
            const auto id = std::to_string(nextInt(lineStream));

            Character ch;
            ch.loc = { nextFloat(lineStream), nextFloat(lineStream) };
            ch.width = nextFloat(lineStream);
            ch.height = nextFloat(lineStream);
            ch.offset = { nextFloat(lineStream), nextFloat(lineStream) };
            ch.xadvance = nextFloat(lineStream);
            ch.page = static_cast<uint32_t>(nextInt(lineStream));

            const auto iter = fontChars.find(id);
            if (iter == fontChars.end()) {
                fontChars.emplace(id, ch);
            }
        }

        if (str == "kerning") {
            uint32_t first = nextInt(lineStream);
            uint32_t second = nextInt(lineStream);
            auto key = fmt::format("{}.{}", first, second);

            float amount = nextFloat(lineStream);

            const auto iter = kerning.find(key);
            if (iter == kerning.end()) {
                kerning.emplace(key, amount);
            }
        }
    }
}

uint32_t OpenGLFont::getLength(std::string_view text, uint32_t targetSize) {
    const auto scale = static_cast<float>(targetSize) / size;
    const auto str =
        text.length() > maxLength ? text.substr(0, maxLength) : text;

    std::string prev;
    uint32_t x = 0;

    for (const char& c : str) {
        auto index = std::to_string(c);
        auto ch = fontChars[index];
        x += ch.xadvance * scale;
        if (!prev.empty()) {
            const auto key = fmt::format("{}.{}", prev, index);
            x += kerning[key] * scale;
        }
        prev = index;
    }

    return x;
}

void OpenGLFont::render(std::string_view text, const glm::vec2& position,
                        uint32_t targetSize, const glm::vec3& color) {
    const auto fontSize = static_cast<float>(targetSize);
    const float scale = fontSize / size;
    const auto str =
        text.length() > maxLength ? text.substr(0, maxLength) : text;

    std::vector<float> batchVertices;
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

        const auto vertices = std::array<float, 16>{
            xpos,     ypos + h, texx,        texy + texh,  //
            xpos,     ypos,     texx,        texy,         //
            xpos + w, ypos,     texx + texw, texy,         //
            xpos + w, ypos + h, texx + texw, texy + texh   //
        };

        batchVertices.insert(batchVertices.end(), vertices.begin(),
                             vertices.end());

        const auto indices = std::array<uint32_t, 6>{
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

    vao->bind();

    auto shader = OpenGLResourceManager::getShader("text");
    shader->bind();
    shader->setFloat3("textColor", color);
    shader->setFloat("screenPxRange", fontSize / size * 4.0F);

    auto tex = OpenGLResourceManager::getTexture(textureName);
    tex->bind();

    vbo->setData(batchVertices.data(),
                 static_cast<uint32_t>(batchVertices.size() * sizeof(float)));

    ebo->setData(batchIndices.data(),
                 static_cast<uint32_t>(batchIndices.size() * sizeof(uint32_t)));

    glDrawElements(GL_TRIANGLES, static_cast<int32_t>(batchIndices.size()),
                   GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
}

void OpenGLFont::log() const {
    SPONGE_CORE_DEBUG("Font file: INFO face={} size={}", face, size);

    SPONGE_CORE_DEBUG(
        "Font file: COMMON lineHeight={:>3} base={:>3} scaleW={:>2} "
        "scaleH={:>3} pages={:2}",
        lineHeight, base, scaleW, scaleH, pages);

    for (const auto& [key, value] : fontChars) {
        SPONGE_CORE_DEBUG(
            "Font file: CHAR {:>3} x={:>3} y={:>3} width={:>2} height={:>3} "
            "xoffset={:2} yoffset={:3} xadvance={:>2} "
            "page={}",
            key, value.loc.x, value.loc.y, value.width, value.height,
            value.offset.x, value.offset.y, value.xadvance, value.page);
    }
}

}  // namespace sponge::graphics::renderer
