#include "shader.hpp"
#include "logging/log.hpp"
#include "platform/opengl/gl.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <cassert>

namespace sponge::platform::opengl {

Shader::Shader(const std::string& vertexSource,
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

Shader::Shader(const std::string& vertexSource,
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

Shader::~Shader() {
    glDeleteProgram(program);
}

void Shader::bind() const {
    glUseProgram(program);
}

void Shader::unbind() const {
    glUseProgram(0);
}

uint32_t Shader::compileShader(const GLenum type, const std::string& source) {
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

uint32_t Shader::linkProgram(uint32_t vs, uint32_t fs) {
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

uint32_t Shader::linkProgram(uint32_t vs, uint32_t fs, uint32_t gs) {
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

void Shader::setBoolean(const std::string& name, bool value) {
    glUniform1i(getUniformLocation(name), static_cast<int>(value));
}

void Shader::setFloat(const std::string& name, float value) {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setFloat3(const std::string& name, const glm::vec3& value) {
    glUniform3f(getUniformLocation(name), value.x, value.y, value.z);
}

void Shader::setFloat4(const std::string& name, const glm::vec4& value) {
    glUniform4f(getUniformLocation(name), value.x, value.y, value.z, value.a);
}

void Shader::setInteger(const std::string& name, int value) {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE,
                       glm::value_ptr(value));
}

GLint Shader::getUniformLocation(const std::string& name) const {
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

}  // namespace sponge::platform::opengl
