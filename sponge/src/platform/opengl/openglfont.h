#pragma once

#include "platform/opengl/openglbuffer.h"
#include "platform/opengl/openglelementbuffer.h"
#include "platform/opengl/opengltexture.h"
#include "platform/opengl/openglvertexarray.h"
#include <absl/container/flat_hash_map.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <string>

namespace sponge::graphics::renderer {

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
    void render(std::string_view text, const glm::vec2& position,
                uint32_t targetSize, const glm::vec3& color);
    uint32_t getLength(std::string_view text, uint32_t targetSize);

    void log() const;

   private:
    const uint32_t maxLength = 256;
    std::unique_ptr<OpenGLBuffer> vbo;
    std::unique_ptr<OpenGLVertexArray> vao;
    std::unique_ptr<OpenGLElementBuffer> ebo;

    absl::flat_hash_map<std::string, Character> fontChars;
    absl::flat_hash_map<std::string, float> kerning;

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

}  // namespace sponge::graphics::renderer
