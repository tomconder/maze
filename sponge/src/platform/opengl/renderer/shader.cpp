#include "platform/opengl/renderer/shader.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace sponge::platform::opengl::renderer {

Shader::Shader(const ShaderCreateInfo& createInfo) {
    assert(!createInfo.name.empty());
    assert(!createInfo.vertexShaderPath.empty());
    assert(!createInfo.fragmentShaderPath.empty());

    shaderName = createInfo.name;

    SPONGE_GL_INFO("Loading vertex shader file: [{}, {}]", createInfo.name,
                   createInfo.vertexShaderPath);
    const std::string vertexSource = loadSourceFromFile(
        createInfo.assetsFolder + createInfo.vertexShaderPath);
    assert(!vertexSource.empty());

    SPONGE_GL_INFO("Loading fragment shader file: [{}, {}]", createInfo.name,
                   createInfo.fragmentShaderPath);
    const std::string fragmentSource = loadSourceFromFile(
        createInfo.assetsFolder + createInfo.fragmentShaderPath);
    assert(!fragmentSource.empty());

    const uint32_t vs = compileShader(GL_VERTEX_SHADER, vertexSource);
    const uint32_t fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    uint32_t gs = 0;
    if (!createInfo.geometryShaderPath.empty()) {
        SPONGE_GL_INFO("Loading geometry shader file: [{}, {}]",
                       createInfo.name, createInfo.geometryShaderPath);

        const std::string geometrySource = loadSourceFromFile(
            createInfo.assetsFolder + createInfo.geometryShaderPath);
        assert(!geometrySource.empty());

        gs      = compileShader(GL_GEOMETRY_SHADER, geometrySource);
        program = linkProgram(vs, fs, gs);
    } else {
        program = linkProgram(vs, fs);
    }

    glDetachShader(program, vs);
    glDetachShader(program, fs);
    if (!createInfo.geometryShaderPath.empty()) {
        glDetachShader(program, gs);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!createInfo.geometryShaderPath.empty()) {
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

    const char* srcPtr = source.c_str();
    glShaderSource(id, 1, &srcPtr, nullptr);
    glCompileShader(id);

    int32_t result = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            std::vector<GLchar> message(length);
            glGetShaderInfoLog(id, length, &length, message.data());
            SPONGE_GL_ERROR("Shader compiling failed: {0}", message.data());
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
        int length = 0;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            std::vector<GLchar> message(length);
            glGetShaderInfoLog(id, length, &length, message.data());
            SPONGE_GL_ERROR("Shader linking failed: {0}", message.data());
        }
    }

    glValidateProgram(id);

    return id;
}

void Shader::setBoolean(const std::string_view name, const bool value) const {
    glUniform1ui(getUniformLocation(name), value ? 1 : 0);
}

void Shader::setFloat(const std::string_view name, const float value) const {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setFloat2(const std::string_view name,
                       const glm::vec2&       value) const {
    glUniform2f(getUniformLocation(name), value.x, value.y);
}

void Shader::setFloat3(const std::string_view name,
                       const glm::vec3&       value) const {
    glUniform3f(getUniformLocation(name), value.x, value.y, value.z);
}

void Shader::setFloat4(const std::string_view name,
                       const glm::vec4&       value) const {
    glUniform4f(getUniformLocation(name), value.x, value.y, value.z, value.a);
}

void Shader::setInteger(const std::string_view name, const int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setMat4(const std::string_view name,
                     const glm::mat4&       value) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, value_ptr(value));
}

GLint Shader::getUniformLocation(const std::string_view name) const {
    assert(!name.empty());

    if (const auto it = uniformLocations.find(name);
        it != uniformLocations.end()) {
        return it->second;
    }

    // allocate string if not cached
    std::string nameStr(name);
    const auto  location = glGetUniformLocation(program, nameStr.c_str());
    if (location == -1) {
        SPONGE_GL_WARN("Uniform name not found: [{}, {}]", nameStr.c_str(),
                       program);
    }
    uniformLocations.emplace(std::move(nameStr), location);

    return location;
}

std::string Shader::loadSourceFromFile(const std::string& path) {
    assert(!path.empty());

    std::string code;
    if (std::ifstream file(path, std::ios::in | std::ios::binary);
        file.good()) {
        file.seekg(0, std::ios::end);
        const auto size = file.tellg();
        file.seekg(0, std::ios::beg);
        code.resize(size);
        file.read(code.data(), size);
        file.close();
    } else {
        SPONGE_GL_ERROR("Unable to open file: {}", path);
    }

    return code;
}

}  // namespace sponge::platform::opengl::renderer
