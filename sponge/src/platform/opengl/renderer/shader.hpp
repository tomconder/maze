#pragma once

#include "core/file.hpp"
#include "core/stringutils.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <optional>
#include <string>
#include <unordered_map>

namespace sponge::platform::opengl::renderer {
struct ShaderCreateInfo {
    std::string name;
    std::string vertexShaderPath;
    std::string fragmentShaderPath;
    std::string geometryShaderPath;
    std::string assetsFolder = core::File::getResourceDir();
};

class Shader final {
public:
    explicit Shader(const ShaderCreateInfo& createInfo);
    ~Shader();

    void bind() const;
    void unbind() const;

    void setBoolean(std::string_view name, bool value) const;
    void setFloat(std::string_view name, float value) const;
    void setFloat2(std::string_view name, const glm::vec2& value) const;
    void setFloat3(std::string_view name, const glm::vec3& value) const;
    void setFloat4(std::string_view name, const glm::vec4& value) const;
    void setInteger(std::string_view name, int value) const;
    void setMat4(std::string_view name, const glm::mat4& value) const;

    uint32_t getId() const {
        return program;
    }

    const std::string& getName() const {
        return shaderName;
    }

private:
    mutable std::unordered_map<std::string, GLint, core::TransparentStringHash,
                               core::TransparentStringEqual>
        uniformLocations;

    uint32_t    compileShader(GLenum type, const std::string& source);
    uint32_t    linkProgram(uint32_t vs, uint32_t fs,
                            std::optional<uint32_t> gs = std::nullopt);
    std::string loadSourceFromFile(const std::string& path);

    uint32_t    program = 0;
    std::string shaderName;

    GLint getUniformLocation(std::string_view name) const;
};
}  // namespace sponge::platform::opengl::renderer
