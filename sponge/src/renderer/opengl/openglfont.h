#pragma once

#include "renderer/opengl/openglbuffer.h"
#include "renderer/opengl/openglelementbuffer.h"
#include "renderer/opengl/opengltexture.h"
#include "renderer/opengl/openglvertexarray.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <unordered_map>

namespace sponge {

struct Character {
    glm::vec2 loc;
    float width;
    float height;
    glm::vec2 offset;
    float xadvance;
    uint32_t page;
};

class OpenGLFont {
   public:
    OpenGLFont();
    void load(const std::string& path);
    void render(const std::string& text, const glm::vec2& position,
                uint32_t targetSize, const glm::vec3& color);

    void log() const;

   private:
    const uint32_t maxLength = 256;
    std::unique_ptr<OpenGLBuffer> vbo;
    std::unique_ptr<OpenGLVertexArray> vao;
    std::unique_ptr<OpenGLElementBuffer> ebo;

    std::unordered_map<glm::uint32, Character> fontChars;
    std::unordered_map<std::string, float> kerning;

    // the name of font
    std::string face;
    // the size of the font
    float size = 24.F;

    // the distance in pixels between each line of text
    float lineHeight = 0.F;
    // the number of pixels from the absolute top of the line to the base of the
    // characters
    float base = 0.F;
    // the width of the texture, normally used to scale the x pos of the
    // character image
    float scaleW = 1.F;
    // the height of the texture, normally used to scale the y pos of the
    // character image
    float scaleH = 1.F;

    // the number of texture pages included in the font
    uint32_t pages = 0;
    // the name of the texture page in the resource manager
    std::string textureName;
};

}  // namespace sponge

#define UNUSED(x) (void)(x)
