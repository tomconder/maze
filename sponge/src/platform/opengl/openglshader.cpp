#include "platform/opengl/openglshader.hpp"
#include "core/log.hpp"
#include "platform/opengl/gl.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <cassert>

namespace sponge::renderer {

OpenGLShader::OpenGLShader(const std::string& vertexSource,
                           const std::string& fragmentSource) {
    assert(!vertexSource.empty());
    assert(!fragmentSource.empty());

    uint32_t vs = compileShader(GL_VERTEX_SHADER, vertexSource);
    uint32_t fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    program = linkProgram(vs, fs);

    glDetachShader(program, vs);
    glDetachShader(program, fs);

    glDeleteShader(vs);
    glDeleteShader(fs);
}

OpenGLShader::OpenGLShader(const std::string& vertexSource,
                           const std::string& fragmentSource,
                           const std::string& geometrySource) {
    assert(!vertexSource.empty());
    assert(!fragmentSource.empty());
    assert(!geometrySource.empty());

    uint32_t vs = compileShader(GL_VERTEX_SHADER, vertexSource);
    uint32_t fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    uint32_t gs = compileShader(GL_GEOMETRY_SHADER, geometrySource);

    program = linkProgram(vs, fs, gs);

    glDetachShader(program, vs);
    glDetachShader(program, fs);
    glDetachShader(program, gs);

    glDeleteShader(vs);
    glDeleteShader(fs);
    glDeleteShader(gs);
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

uint32_t OpenGLShader::compileShader(const GLenum type,
                                     const std::string& source) {
    uint32_t id = glCreateShader(type);
    assert(id != 0);

    const char* shader = source.c_str();
    glShaderSource(id, 1, &shader, nullptr);
    glCompileShader(id);

    int32_t result = GL_FALSE;
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

uint32_t OpenGLShader::linkProgram(uint32_t vs, uint32_t fs) {
    uint32_t id = glCreateProgram();

    glAttachShader(id, vs);
    glAttachShader(id, fs);

    glLinkProgram(id);

    int32_t result = GL_FALSE;

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

uint32_t OpenGLShader::linkProgram(uint32_t vs, uint32_t fs, uint32_t gs) {
    uint32_t id = glCreateProgram();

    glAttachShader(id, vs);
    glAttachShader(id, fs);
    glAttachShader(id, gs);

    glLinkProgram(id);

    int32_t result = GL_FALSE;

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

void OpenGLShader::setBoolean(const std::string& uname, bool value) {
    glUniform1i(getUniformLocation(uname), static_cast<int>(value));
}

void OpenGLShader::setFloat(const std::string& uname, float value) {
    glUniform1f(getUniformLocation(uname), value);
}

void OpenGLShader::setFloat3(const std::string& uname, const glm::vec3& value) {
    glUniform3f(getUniformLocation(uname), value.x, value.y, value.z);
}

void OpenGLShader::setFloat4(const std::string& uname, const glm::vec4& value) {
    glUniform4f(getUniformLocation(uname), value.x, value.y, value.z, value.a);
}

void OpenGLShader::setInteger(const std::string& uname, int value) {
    glUniform1i(getUniformLocation(uname), value);
}

void OpenGLShader::setMat4(const std::string& uname, const glm::mat4& value) {
    glUniformMatrix4fv(getUniformLocation(uname), 1, GL_FALSE,
                       glm::value_ptr(value));
}

GLint OpenGLShader::getUniformLocation(const std::string& name) const {
    assert(!name.empty());

    if (uniformLocations.find(name) != uniformLocations.end()) {
        return uniformLocations[name];
    }

    const auto location = glGetUniformLocation(program, name.c_str());
    if (location == -1) {
        SPONGE_CORE_WARN("Uniform name not found: [{}, {}]", name.c_str(),
                         program);
    }
    uniformLocations[name] = location;

    return location;
}

}  // namespace sponge::renderer
