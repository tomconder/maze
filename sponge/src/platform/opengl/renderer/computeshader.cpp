// sponge/src/platform/opengl/renderer/computeshader.cpp
#include "platform/opengl/renderer/computeshader.hpp"

#include "core/file.hpp"
#include "platform/opengl/renderer/gl.hpp"
#include "platform/opengl/renderer/shaderutils.hpp"

#include <cassert>
#include <vector>

namespace sponge::platform::opengl::renderer {

ComputeShader::ComputeShader(const std::string& name,
                             const std::string& glslPath) {
    shaderName = name;

    const std::string source =
        loadGlslSource(core::File::getResourceDir() + glslPath);
    assert(!source.empty());

    const char*    srcPtr = source.c_str();
    const uint32_t cs     = glCreateShader(GL_COMPUTE_SHADER);
    assert(cs != 0);

    glShaderSource(cs, 1, &srcPtr, nullptr);
    glCompileShader(cs);

    int32_t result = GL_FALSE;
    glGetShaderiv(cs, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length = 0;
        glGetShaderiv(cs, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            std::vector<GLchar> msg(length);
            glGetShaderInfoLog(cs, length, &length, msg.data());
            SPONGE_GL_ERROR("Compute shader compile failed [{}]: {}", name,
                            msg.data());
        }
        glDeleteShader(cs);
        return;
    }

    program = glCreateProgram();
    glAttachShader(program, cs);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &result);
    if (result == GL_FALSE) {
        int length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            std::vector<GLchar> msg(length);
            glGetProgramInfoLog(program, length, &length, msg.data());
            SPONGE_GL_ERROR("Compute shader link failed [{}]: {}", name,
                            msg.data());
        }
        glDeleteProgram(program);
        program = 0;
    }

    if (program != 0) {
        glDetachShader(program, cs);
    }
    glDeleteShader(cs);
}

ComputeShader::~ComputeShader() {
    if (program != 0) {
        glDeleteProgram(program);
    }
}

ComputeShader::ComputeShader(ComputeShader&& other) noexcept {
    program       = other.program;
    shaderName    = std::move(other.shaderName);
    other.program = 0;
}

ComputeShader& ComputeShader::operator=(ComputeShader&& other) noexcept {
    if (this != &other) {
        if (program != 0) {
            glDeleteProgram(program);
        }
        program       = other.program;
        shaderName    = std::move(other.shaderName);
        other.program = 0;
    }
    return *this;
}

void ComputeShader::dispatch(const uint32_t groupsX, const uint32_t groupsY,
                             const uint32_t groupsZ) const {
    assert(program != 0);
    glUseProgram(program);
    glDispatchCompute(groupsX, groupsY, groupsZ);
    glUseProgram(0);
}

}  // namespace sponge::platform::opengl::renderer
