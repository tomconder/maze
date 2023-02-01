#include "openglbmfont.h"

#ifdef EMSCRIPTEN
#include <new>
#endif

#include <fstream>
#include <glm/ext/matrix_clip_space.hpp>
#include <sstream>

#include "core/log.h"
#include "openglresourcemanager.h"

OpenGLBMFont::OpenGLBMFont(int screenWidth, int screenHeight) {
    auto shader = OpenGLResourceManager::getShader("bmtext");
    shader->bind();

    auto projection = glm::ortho(0.f, static_cast<float>(screenWidth), 0.f, static_cast<float>(screenHeight));
    shader->setMat4("projection", projection);

    vao = std::make_unique<OpenGLVertexArray>();
    vao->bind();

    vbo = std::make_unique<OpenGLBuffer>(static_cast<GLuint>(sizeof(float) * 16));
    vbo->bind();

    ebo = std::make_unique<OpenGLElementBuffer>(static_cast<GLuint>(sizeof(GLuint) * 6));
    ebo->bind();

    GLuint program = shader->getId();
    auto position = static_cast<GLuint>(glGetAttribLocation(program, "vertex"));
    glEnableVertexAttribArray(position);
    glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void OpenGLBMFont::load(const std::string& path) {
    assert(!path.empty());

    SPONGE_CORE_DEBUG("loadFromBMFile: {}", path);

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
            glm::uint32 id = nextInt(lineStream);
            std::string name = nextString(lineStream);

            auto pos = path.find_last_of('/');
            auto fontFolder = path.substr(0, pos + 1);
            textureName = "font";
            auto texture = OpenGLResourceManager::loadTexture(fontFolder + name, textureName);
        }

        if (str == "char") {
            glm::uint32 id = nextInt(lineStream);
            fontChars[id].loc.x = nextFloat(lineStream);
            fontChars[id].loc.y = nextFloat(lineStream);
            fontChars[id].width = nextFloat(lineStream);
            fontChars[id].height = nextFloat(lineStream);
            fontChars[id].offset.x = nextFloat(lineStream);
            fontChars[id].offset.y = nextFloat(lineStream);
            fontChars[id].xadvance = nextInt(lineStream);
            fontChars[id].page = nextInt(lineStream);
        }
    }
}

void OpenGLBMFont::renderText(const std::string& text, float x, float y, Uint32 targetSize, glm::vec3 color) {
    auto fontSize = static_cast<float>(targetSize);

    auto shader = OpenGLResourceManager::getShader("bmtext");
    shader->bind();
    shader->setFloat3("textColor", color);
    shader->setFloat("screenPxRange", fontSize / size * 4.0f);

    glActiveTexture(GL_TEXTURE0);
    auto tex = OpenGLResourceManager::getTexture(textureName);
    tex->bind();

    vao->bind();

    float scale = fontSize / size;

    for (const char& c : text) {
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

        auto indices = std::vector<GLuint>{
            0, 1, 2,  //
            0, 2, 3   //
        };

        ebo->bind();
        ebo->setData(indices.data(), (GLuint)indices.size() * (GLuint)sizeof(GLuint));

        vbo->bind();
        glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawElements(GL_TRIANGLES, (GLint)indices.size(), GL_UNSIGNED_INT, nullptr);

        x += static_cast<float>(ch.xadvance) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLBMFont::log() const {
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
