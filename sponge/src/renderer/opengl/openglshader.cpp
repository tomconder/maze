#include "openglshader.h"

#include <cassert>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>

OpenGLShader::OpenGLShader(const std::string &name, const std::string &vertexSource, const std::string &fragmentSource)
    : name(name) {
    assert(!vertexSource.empty());
    assert(!fragmentSource.empty());

    SPONGE_CORE_DEBUG("Building program for shader = {}", name);

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    program = linkProgram(vs, fs);

    glDetachShader(program, vs);
    glDetachShader(program, fs);

    glDeleteShader(vs);
    glDeleteShader(fs);
}

OpenGLShader::~OpenGLShader() {
    glDeleteProgram(program);
}

void OpenGLShader::bind() const {
    glUseProgram(program);
}

void OpenGLShader::unbind() const {
    glUseProgram(0);
}

GLuint OpenGLShader::compileShader(const GLenum type, const std::string &source) {
    GLuint id = glCreateShader(type);
    assert(id != 0);

    if (type == GL_VERTEX_SHADER) {
        SPONGE_CORE_DEBUG("Compiling vertex shader for shader = {}", name);
    } else {
        SPONGE_CORE_DEBUG("Compiling fragment shader for shader = {}", name);
    }

    char const *shader = source.c_str();
    glShaderSource(id, 1, &shader, nullptr);
    glCompileShader(id);

    GLint result = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            std::vector<GLchar> message(length);
            glGetShaderInfoLog(id, length, &length, message.data());
            SPONGE_CORE_ERROR("Shader compiling failed: {0}", message.data());
            glDeleteShader(id);
            return 0;
        }
    }

    return id;
}

GLuint OpenGLShader::linkProgram(GLuint vs, GLuint fs) {
    GLuint id = glCreateProgram();

    glAttachShader(id, vs);
    glAttachShader(id, fs);

    SPONGE_CORE_DEBUG("Linking shader [{}]: program = {}", name, id);

    glLinkProgram(id);

    GLint result = GL_FALSE;

    glGetProgramiv(id, GL_LINK_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            std::vector<GLchar> message(length);
            glGetShaderInfoLog(id, length, &length, message.data());
            SPONGE_CORE_ERROR("Shader linking failed: {0}", message.data());
        }
    }

    glValidateProgram(id);

    return id;
}

void OpenGLShader::setBoolean(const std::string &uname, bool value) {
    glUniform1i(glGetUniformLocation(program, uname.c_str()), static_cast<int>(value));
}

void OpenGLShader::setFloat(const std::string &uname, float value) {
    glUniform1f(glGetUniformLocation(program, uname.c_str()), value);
}

void OpenGLShader::setFloat3(const std::string &uname, const glm::vec3 &value) {
    glUniform3f(glGetUniformLocation(program, uname.c_str()), value.x, value.y, value.z);
}

void OpenGLShader::setInteger(const std::string &uname, int value) {
    glUniform1i(glGetUniformLocation(program, uname.c_str()), value);
}

void OpenGLShader::setMat4(const std::string &uname, const glm::mat4 &value) {
    glUniformMatrix4fv(glGetUniformLocation(program, uname.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}
