#include "shader.hpp"
#include "logging/log.hpp"
#include "platform/opengl/renderer/gl.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <cassert>

namespace sponge::platform::opengl::renderer {

Shader::Shader(const std::string& name, const std::string& vertexSource,
               const std::string& fragmentSource,
               const std::optional<std::string>& geometrySource) {
    assert(!name.empty());
    assert(!vertexSource.empty());
    assert(!fragmentSource.empty());

    this->name = name;

    const uint32_t vs = compileShader(GL_VERTEX_SHADER, vertexSource);
    const uint32_t fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    uint32_t gs = 0;
    if (geometrySource) {
        gs = compileShader(GL_GEOMETRY_SHADER, *geometrySource);
    }

    if (geometrySource) {
        program = linkProgram(vs, fs, gs);
    } else {
        program = linkProgram(vs, fs);
    }

    glDetachShader(program, vs);
    glDetachShader(program, fs);
    if (geometrySource) {
        glDetachShader(program, gs);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    if (geometrySource) {
        glDeleteShader(gs);
    }
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
    const uint32_t id = glCreateShader(type);
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

uint32_t Shader::linkProgram(const uint32_t vs, const uint32_t fs,
                             const std::optional<uint32_t> gs) {
    const uint32_t id = glCreateProgram();

    glAttachShader(id, vs);
    glAttachShader(id, fs);
    if (gs) {
        glAttachShader(id, *gs);
    }

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

void Shader::setBoolean(const std::string& name, const bool value) {
    glUniform1i(getUniformLocation(name), static_cast<int>(value));
}

void Shader::setFloat(const std::string& name, const float value) {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setFloat3(const std::string& name, const glm::vec3& value) {
    glUniform3f(getUniformLocation(name), value.x, value.y, value.z);
}

void Shader::setFloat4(const std::string& name, const glm::vec4& value) {
    glUniform4f(getUniformLocation(name), value.x, value.y, value.z, value.a);
}

void Shader::setInteger(const std::string& name, const int value) {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, value_ptr(value));
}

GLint Shader::getUniformLocation(const std::string& name) const {
    assert(!name.empty());

    if (uniformLocations.contains(name)) {
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

}  // namespace sponge::platform::opengl::renderer
