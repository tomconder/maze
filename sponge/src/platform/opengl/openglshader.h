#pragma once

#include "graphics/renderer/shader.h"
#include "platform/opengl/gl.h"
#include <string>

namespace sponge::graphics::renderer {

class OpenGLShader : public Shader {
   public:
    OpenGLShader(const std::string& vertexSource,
                 const std::string& fragmentSource);
    ~OpenGLShader() override;

    void bind() const override;
    void unbind() const override;

    void setBoolean(const std::string& name, bool value) override;
    void setFloat(const std::string& name, float value) override;
    void setFloat3(const std::string& name, const glm::vec3& value) override;
    void setFloat4(const std::string& name, const glm::vec4& value) override;
    void setInteger(const std::string& name, int value) override;
    void setMat4(const std::string& name, const glm::mat4& value) override;

    uint32_t getId() const {
        return program;
    };

   private:
    uint32_t compileShader(GLenum type, const std::string& file);
    uint32_t linkProgram(uint32_t vs, uint32_t fs);

    uint32_t program = 0;
};

}  // namespace sponge::graphics::renderer
