#pragma once

#include "core/file.hpp"
#include "core/stringutils.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

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

    uint32_t    compileShader(GLenum type, const std::string& source) const;
    uint32_t    linkProgram(uint32_t vs, uint32_t fs,
                            std::optional<uint32_t> gs = std::nullopt) const;
    std::string loadSourceFromFile(const std::string& path) const;

    uint32_t    program = 0;
    std::string shaderName;

    GLint getUniformLocation(std::string_view name) const;

    // UBO support for Slang-generated shaders
    struct UBOBlock {
        uint32_t                               buffer  = 0;
        uint32_t                               binding = 0;
        GLsizei                                size    = 0;
        std::unordered_map<std::string, GLint> offsets;
        mutable std::vector<uint8_t>           staging;
        mutable bool                           dirty = false;
    };
    std::vector<UBOBlock> uboBlocks;
    mutable bool          isBound = false;

    void initUBO();
    void uploadUBO() const;
    bool trySetInUBO(std::string_view name, const void* data, size_t bytes,
                     size_t typeSize) const;
};
}  // namespace sponge::platform::opengl::renderer
