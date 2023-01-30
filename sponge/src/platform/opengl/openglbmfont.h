#pragma once

#include <glm/vec2.hpp>
#include <memory>
#include <string>
#include <unordered_map>

#include "openglbuffer.h"
#include "opengltexture.h"
#include "openglvertexarray.h"

struct BMCharacter {
    glm::u32vec2 loc;
    glm::uint32 width;
    glm::uint32 height;
    glm::i32vec2 offset;
    glm::int32 xadvance;
    glm::uint32 page;
};

class OpenGLBMFont {
   public:
    OpenGLBMFont(int screenWidth, int screenHeight);
    void load(const std::string &path);

    // TODO: render
    // void renderText(const std::string &text, float x, float y, glm::vec3 color);

    std::string textureFileName;

    std::unique_ptr<OpenGLBuffer> vbo;
    std::unique_ptr<OpenGLVertexArray> vao;

    std::unordered_map<glm::uint32, BMCharacter> fontChars;

   private:
    void logChars() const;
};
