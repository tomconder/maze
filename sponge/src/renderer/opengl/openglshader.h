#pragma once

#include <string>

#include "renderer/opengl/gl.h"
#include "renderer/shader.h"

namespace Sponge {

class OpenGLShader : public Shader {
   public:
    OpenGLShader(const std::string &name, const std::string &vertexSource, const std::string &fragmentSource);
    ~OpenGLShader() override;

    void bind() const override;
    void unbind() const override;

    void setBoolean(const std::string &name, bool value) override;
    void setFloat(const std::string &name, float value) override;
    void setFloat3(const std::string &name, const glm::vec3 &value) override;
    void setInteger(const std::string &name, int value) override;
    void setMat4(const std::string &name, const glm::mat4 &value) override;

    const std::string &getName() const override {
        return name;
    };

    GLuint getId() const {
        return program;
    };

   private:
    GLuint compileShader(GLenum type, const std::string &file);
    GLuint linkProgram(GLuint vs, GLuint fs);

    GLuint program = 0;
    std::string name;
};

}  // namespace Sponge
