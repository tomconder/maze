#pragma once

#include <string>

#include "gl.h"
#include "renderer/shader.h"

class OpenGLShader : public Shader {
   public:
    OpenGLShader(const std::string &name, const std::string &vertexSource, const std::string &fragmentSource);
    virtual ~OpenGLShader() override;

    virtual void bind() const override;
    virtual void unbind() const override;

    virtual void setBoolean(const std::string &name, bool value) override;
    virtual void setFloat(const std::string &name, float value) override;
    virtual void setFloat3(const std::string &name, const glm::vec3 &value) override;
    virtual void setInteger(const std::string &name, int value) override;
    virtual void setMat4(const std::string &name, const glm::mat4 &value) override;

    virtual const std::string &getName() const override {
        return name;
    };

    GLuint getId() const {
        return program;
    };

   private:
    static GLuint compileShader(GLenum type, const std::string &file);
    static GLuint linkProgram(GLuint vs, GLuint fs);

    GLuint program = 0;
    std::string name;
};
