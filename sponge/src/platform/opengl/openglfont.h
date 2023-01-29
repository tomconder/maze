#pragma once

#include <ft2build.h>

#include <memory>
#include <string>
#include <unordered_map>
#include FT_FREETYPE_H

#include <SDL.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "openglbuffer.h"
#include "opengltexture.h"
#include "openglvertexarray.h"

struct Character {
    unsigned int id;
    glm::ivec2 size;
    glm::ivec2 bearing;
    unsigned int advance;
};

class OpenGLFont {
   public:
    OpenGLFont(int screenWidth, int screenHeight);
    void load(const std::string &path, unsigned int fontSize);
    void loadFromBMFile(const std::string &path);

    void renderText(const std::string &text, float x, float y, glm::vec3 color);

    std::unique_ptr<OpenGLBuffer> vbo;
    std::unique_ptr<OpenGLVertexArray> vao;

    std::unordered_map<unsigned char, Character> Characters;
};
