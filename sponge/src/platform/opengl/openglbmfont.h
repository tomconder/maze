#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <memory>
#include <string>
#include <unordered_map>

#include "openglbuffer.h"
#include "openglelementbuffer.h"
#include "opengltexture.h"
#include "openglvertexarray.h"

struct BMCharacter {
    glm::vec2 loc;
    float width;
    float height;
    glm::vec2 offset;
    glm::int32 xadvance;
    glm::uint32 page;
};

class OpenGLBMFont {
   public:
    OpenGLBMFont(int screenWidth, int screenHeight);
    void load(const std::string &path);
    void renderText(const std::string &text, float x, float y, Uint32 targetSize, glm::vec3 color);

    void log() const;

   private:
    std::unique_ptr<OpenGLBuffer> vbo;
    std::unique_ptr<OpenGLVertexArray> vao;
    std::unique_ptr<OpenGLElementBuffer> ebo;

    std::unordered_map<glm::uint32, BMCharacter> fontChars;

    // the name of font
    std::string face;
    // the size of the font
    float size;

    // the distance in pixels between each line of text
    float lineHeight;
    // the number of pixels from the absolute top of the line to the base of the characters
    float base;
    // the width of the texture, normally used to scale the x pos of the character image
    float scaleW;
    // the height of the texture, normally used to scale the y pos of the character image
    float scaleH;

    // the number of texture pages included in the font
    glm::uint32 pages;
    // the name of the texture page in the resource manager
    std::string textureName;
};
