#pragma once

#include "core/file.hpp"
#include "core/stringutils.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace sponge::platform::opengl::renderer {
struct ShaderCreateInfo {
    std::string name;
    std::string vertexShaderPath   = "";
    std::string fragmentShaderPath = "";
    std::string geometryShaderPath = "";
    std::string computeShaderPath  = "";
    std::string assetsFolder       = core::File::getResourceDir();
};

class Shader final {
public:
    explicit Shader(const ShaderCreateInfo& createInfo);
    ~Shader();

    void bind() const;
    void unbind() const;

    // Dispatch groupsX * groupsY * groupsZ work groups (compute shaders only).
    // Caller must issue glMemoryBarrier after if SSBOs are read by later
    // stages.
    void dispatch(uint32_t groupsX, uint32_t groupsY = 1,
                  uint32_t groupsZ = 1) const;

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
                               std::equal_to<>>
        uniformLocations;

    uint32_t    compileShader(GLenum type, const std::string& source) const;
    uint32_t    compileStage(GLenum type, std::string_view stageName,
                             const std::string& assetsFolder,
                             const std::string& path) const;
    uint32_t    linkProgram(uint32_t vs, uint32_t fs = 0,
                            std::optional<uint32_t> gs = std::nullopt) const;
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
