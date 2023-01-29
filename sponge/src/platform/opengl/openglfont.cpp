#include "openglfont.h"

#ifdef EMSCRIPTEN
#include <new>
#endif

#include <glm/ext/matrix_clip_space.hpp>
#include <regex>
#include <sstream>

#include "core/log.h"
#include "openglresourcemanager.h"
#include "spdlog/fmt/bundled/ranges.h"

OpenGLFont::OpenGLFont(int screenWidth, int screenHeight) {
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

void OpenGLFont::load(const std::string &path, unsigned int fontSize) {
    assert(!path.empty());

    Characters.clear();

    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        SPONGE_CORE_ERROR("Unable to initialize FreeType library");
        FT_Done_FreeType(ft);
        return;
    }

    FT_Face face;
    if (FT_New_Face(ft, path.c_str(), 0, &face)) {
        SPONGE_CORE_ERROR("Unable to load font: {}", path.c_str());
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // disable byte-alignment restriction

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            SPONGE_CORE_ERROR("Unable to load glyph: {}", c);
            continue;
        }

        unsigned int id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, static_cast<int>(face->glyph->bitmap.width),
                     static_cast<int>(face->glyph->bitmap.rows), 0, GL_RED, GL_UNSIGNED_BYTE,
                     face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        Character character = { id, glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                                static_cast<unsigned int>(face->glyph->advance.x) };

        Characters.try_emplace(c, character);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void OpenGLFont::loadFromBMFile(const std::string &path, unsigned int fontSize) {
    assert(!path.empty());

    SPONGE_CORE_DEBUG("loadFromBMFile: {}", path);

    std::string code;
    if (std::ifstream stream(path, std::ios::in | std::ios::binary); stream.is_open()) {
        std::stringstream sstr;
        sstr << stream.rdbuf();
        code = sstr.str();
        std::string s;
        stream.close();
    } else {
        SPONGE_CORE_ERROR("Unable to open file: {}", path);
    }

    auto const re = std::regex{ "\\s+" };
    auto const vec = std::vector<std::string>(std::sregex_token_iterator{ begin(code), end(code), re, -1 },
                                              std::sregex_token_iterator{});
    // SPONGE_CORE_DEBUG("Font file: {}", fmt::join(vec, ", "));

    // id = ch.id
    // x = left position of ch in texture
    // y = top position of ch in texture
    // width = ch.size.x
    // height = ch.size.y
    // xadvance = ch.advance
    // xoffset = ch.bearing.x
    // yoffset = ch.bearing.y

    enum class State { info, common, page, chars, kerning, none };
    State state = State::none;

    std::vector<std::string> info;
    std::vector<std::string> common;
    std::vector<std::string> page;
    std::vector<std::string> chars;
    std::vector<std::string> kernings;

    for (auto i : vec) {
        if (i == "info") {
            state = State::info;
        } else if (i == "common") {
            state = State::common;
        } else if (i == "page") {
            state = State::page;
        } else if (i == "chars") {
            state = State::chars;
        } else if (i == "kernings") {
            state = State::kerning;
        } else {
            if (state == State::info) {
                info.push_back(i);
            } else if (state == State::common) {
                common.push_back(i);
            } else if (state == State::page) {
                page.push_back(i);
            } else if (state == State::chars) {
                if (i.rfind("count=", 0) == 0) {
                    size_t pos = i.find_last_of('=');
                    if (pos != std::string::npos) {
                        auto size = std::stoi(i.substr(pos + 1));
                        SPONGE_CORE_DEBUG("Font file: CHARS reserving size = {}", size);
                        chars.reserve(size);
                    }
                } else {
                    chars.push_back(i);
                }
            } else if (state == State::kerning) {
                if (i.rfind("count=", 0) == 0) {
                    size_t pos = i.find_last_of('=');
                    if (pos != std::string::npos) {
                        auto size = std::stoi(i.substr(pos + 1));
                        SPONGE_CORE_DEBUG("Font file: KERNINGS reserving size = {}", size);
                        kernings.reserve(size);
                    }
                } else {
                    kernings.push_back(i);
                }
            }
        }
    }

    SPONGE_CORE_DEBUG("Font file: INFO {}", fmt::join(info, ", "));
    SPONGE_CORE_DEBUG("Font file: COMMON {}", fmt::join(common, ", "));
    SPONGE_CORE_DEBUG("Font file: PAGE {}", fmt::join(page, ", "));
    SPONGE_CORE_DEBUG("Font file: CHAR {}", fmt::join(chars, ", "));
    SPONGE_CORE_DEBUG("Font file: KERNING {}", fmt::join(kernings, ", "));
}

void OpenGLFont::renderText(const std::string &text, float x, float y, glm::vec3 color) {
    auto shader = OpenGLResourceManager::getShader("text");
    shader->bind();
    shader->setFloat3("textColor", color);

    glActiveTexture(GL_TEXTURE0);
    vao->bind();

    float scale = 1.f;

    for (const char &c : text) {
        Character ch = Characters[c];

        float xpos = x + static_cast<float>(ch.bearing.x) * scale;
        float ypos = y - static_cast<float>(ch.size.y - ch.bearing.y) * scale;

        float w = static_cast<float>(ch.size.x) * scale;
        float h = static_cast<float>(ch.size.y) * scale;

        float vertices[6][4] = { { xpos, ypos + h, 0.0f, 0.0f },    { xpos, ypos, 0.0f, 1.0f },
                                 { xpos + w, ypos, 1.0f, 1.0f },

                                 { xpos, ypos + h, 0.0f, 0.0f },    { xpos + w, ypos, 1.0f, 1.0f },
                                 { xpos + w, ypos + h, 1.0f, 0.0f } };

        glBindTexture(GL_TEXTURE_2D, ch.id);

        vbo->bind();
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += static_cast<float>(ch.advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
