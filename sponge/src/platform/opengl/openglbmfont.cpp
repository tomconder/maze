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
    auto shader = OpenGLResourceManager::getShader("text");
    shader->bind();

    auto projection = glm::ortho(0.f, static_cast<float>(screenWidth), 0.f, static_cast<float>(screenHeight));
    shader->setMat4("projection", projection);

    vao = std::make_unique<OpenGLVertexArray>();
    vao->bind();

    vbo = std::make_unique<OpenGLBuffer>((GLuint)sizeof(float) * 6 * 4);
    vbo->bind();

    GLuint program = shader->getId();
    auto position = static_cast<GLuint>(glGetAttribLocation(program, "vertex"));
    glEnableVertexAttribArray(position);
    glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void OpenGLBMFont::load(const std::string& path) {
    assert(!path.empty());

    SPONGE_CORE_DEBUG("loadFromBMFile: {}", path);

    std::ifstream stream(path, std::ios::in | std::ios::binary);
    assert(stream.good());

    auto nextValue = [](std::stringstream& sstream) {
        std::string s;
        sstream >> s;
        if (size_t pos = s.find_last_of('='); pos != std::string::npos) {
            return std::stoi(s.substr(pos + 1));
        }
        return 0;
    };

    while (!stream.eof()) {
        std::string line;
        std::stringstream lineStream;
        std::getline(stream, line);
        lineStream << line;

        std::string info;
        lineStream >> info;

        if (info == "chars") {
            glm::uint32 size = nextValue(lineStream);
            SPONGE_CORE_DEBUG("Reserving char map to size {}", size);
            fontChars.reserve(size);
        }

        if (info == "char") {
            glm::uint32 id = nextValue(lineStream);
            fontChars[id].loc.x = nextValue(lineStream);
            fontChars[id].loc.y = nextValue(lineStream);
            fontChars[id].width = nextValue(lineStream);
            fontChars[id].height = nextValue(lineStream);
            fontChars[id].offset.x = nextValue(lineStream);
            fontChars[id].offset.y = nextValue(lineStream);
            fontChars[id].xadvance = nextValue(lineStream);
            fontChars[id].page = nextValue(lineStream);
        }
    }

    logChars();
}

void OpenGLBMFont::logChars() const {
    for (const auto& [key, value] : fontChars) {
        SPONGE_CORE_DEBUG(
            "Font file: CHAR {:>3} x={:>3} y={:>3} width={:>2} height={:>3} xoffset={:2} yoffset={:3} xadvance={:>2} "
            "page={}",
            key, value.loc.x, value.loc.y, value.width, value.height, value.offset.x, value.offset.y, value.xadvance,
            value.page);
    }
}
