#pragma once

#include "core/file.hpp"
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
    Shader(const ShaderCreateInfo& createInfo);
    ~Shader();

    void bind() const;
    void unbind() const;

    void setBoolean(const std::string& name, bool value);
    void setFloat(const std::string& name, float value);
    void setFloat3(const std::string& name, const glm::vec3& value);
    void setFloat4(const std::string& name, const glm::vec4& value);
    void setInteger(const std::string& name, int value);
    void setMat4(const std::string& name, const glm::mat4& value);

    uint32_t getId() const {
        return program;
    }

    const std::string& getName() const {
        return name;
    }

   private:
    mutable std::unordered_map<std::string, GLint> uniformLocations;

    uint32_t compileShader(GLenum type, const std::string& source);
    uint32_t linkProgram(uint32_t vs, uint32_t fs,
                         std::optional<uint32_t> gs = std::nullopt);
    std::string loadSourceFromFile(const std::string& path);

    uint32_t program = 0;
    std::string name;

    GLint getUniformLocation(const std::string& name) const;
};
}  // namespace sponge::platform::opengl::renderer
