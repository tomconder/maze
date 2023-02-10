#include "openglfont.h"

#ifdef EMSCRIPTEN
#include <new>
#endif

#include <glm/ext/matrix_clip_space.hpp>
#include <sstream>

#include "openglresourcemanager.h"

OpenGLFont::OpenGLFont(int screenWidth, int screenHeight) {
    auto shader = OpenGLResourceManager::getShader("text");
    shader->bind();

    auto projection = glm::ortho(0.f, static_cast<float>(screenWidth), 0.f, static_cast<float>(screenHeight));
    shader->setMat4("projection", projection);

    vao = std::make_unique<OpenGLVertexArray>();
    vao->bind();

    vbo = std::make_unique<OpenGLBuffer>(maxLength * static_cast<uint32_t>(sizeof(float)) * 16);
    vbo->bind();

    ebo = std::make_unique<OpenGLElementBuffer>(maxLength * static_cast<uint32_t>(sizeof(uint32_t)) * 6);
    ebo->bind();

    uint32_t program = shader->getId();
    auto position = static_cast<uint32_t>(glGetAttribLocation(program, "vertex"));
    glEnableVertexAttribArray(position);
    glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
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
        return 0.f;
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
            std::string name = nextString(lineStream);

            auto pos = path.find_last_of('/');
            auto fontFolder = path.substr(0, pos + 1);
            textureName = "font";
            auto texture = OpenGLResourceManager::loadTexture(fontFolder + name, textureName);
        }

        if (str == "char") {
            uint32_t id = nextInt(lineStream);
            fontChars[id].loc.x = nextFloat(lineStream);
            fontChars[id].loc.y = nextFloat(lineStream);
            fontChars[id].width = nextFloat(lineStream);
            fontChars[id].height = nextFloat(lineStream);
            fontChars[id].offset.x = nextFloat(lineStream);
            fontChars[id].offset.y = nextFloat(lineStream);
            fontChars[id].xadvance = nextFloat(lineStream);
            fontChars[id].page = nextInt(lineStream);
        }

        if (str == "kerning") {
            uint32_t first = nextInt(lineStream);
            uint32_t second = nextInt(lineStream);
            float amount = nextFloat(lineStream);
            std::string key = std::to_string(first) + "." + std::to_string(second);
            kerning[key] = amount;
        }
    }
}

void OpenGLFont::renderText(const std::string& text, float x, float y, uint32_t targetSize, glm::vec3 color) {
    const auto fontSize = static_cast<float>(targetSize);
    const float scale = fontSize / size;
    const std::string str = text.length() > maxLength ? text.substr(0, maxLength) : text;

    std::vector<float> batchVertices;
    std::vector<uint32_t> batchIndices;
    uint32_t numIndices = 0;
    char prev = 0;

    for (const char& c : str) {
        auto ch = fontChars[c];

        auto xpos = x + ch.offset.x * scale;
        auto ypos = y - (ch.height + ch.offset.y) * scale;

        auto w = ch.width * scale;
        auto h = ch.height * scale;

        auto texx = ch.loc.x / scaleW;
        auto texy = ch.loc.y / scaleH;
        auto texh = ch.height / scaleH;
        auto texw = ch.width / scaleW;

        auto vertices = std::vector<float>{
            xpos,     ypos + h, texx,        texy,         //
            xpos,     ypos,     texx,        texy + texh,  //
            xpos + w, ypos,     texx + texw, texy + texh,  //
            xpos + w, ypos + h, texx + texw, texy          //
        };

        batchVertices.insert(batchVertices.end(), vertices.begin(), vertices.end());

        auto indices = std::vector<uint32_t>{
            numIndices, numIndices + 1, numIndices + 2,  //
            numIndices, numIndices + 2, numIndices + 3   //
        };

        batchIndices.insert(batchIndices.end(), indices.begin(), indices.end());

        x += ch.xadvance * scale;
        numIndices += 4;

        if (prev != 0) {
            x += kerning[std::to_string(prev) + "." + std::to_string(c)] * scale;
        }
        prev = c;
    }

    vao->bind();

    auto shader = OpenGLResourceManager::getShader("text");
    shader->bind();
    shader->setFloat3("textColor", color);
    shader->setFloat("screenPxRange", fontSize / size * 4.0f);

    glActiveTexture(GL_TEXTURE0);
    auto tex = OpenGLResourceManager::getTexture(textureName);
    tex->bind();

    vbo->bind();
    vbo->setData(batchVertices.data(), static_cast<GLsizeiptr>(batchVertices.size() * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    ebo->bind();
    ebo->setData(batchIndices.data(), static_cast<GLsizeiptr>(batchIndices.size() * sizeof(uint32_t)));

    glDrawElements(GL_TRIANGLES, (GLint)batchIndices.size(), GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLFont::log() const {
    SPONGE_CORE_DEBUG("Font file: INFO face={} size={}", face, size);

    SPONGE_CORE_DEBUG("Font file: COMMON lineHeight={:>3} base={:>3} scaleW={:>2} scaleH={:>3} pages={:2}", lineHeight,
                      base, scaleW, scaleH, pages);

    for (const auto& [key, value] : fontChars) {
        SPONGE_CORE_DEBUG(
            "Font file: CHAR {:>3} x={:>3} y={:>3} width={:>2} height={:>3} xoffset={:2} yoffset={:3} xadvance={:>2} "
            "page={}",
            key, value.loc.x, value.loc.y, value.width, value.height, value.offset.x, value.offset.y, value.xadvance,
            value.page);
    }
}
