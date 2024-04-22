#pragma once

#include "renderer/shader.hpp"
#include <absl/container/flat_hash_map.h>
#include <string>

namespace sponge::platform::opengl {

class Shader : public renderer::Shader {
   public:
    Shader(const std::string& vertexSource, const std::string& fragmentSource);
    Shader(const std::string& vertexSource, const std::string& fragmentSource,
           const std::string& geometrySource);
    ~Shader() override;

    void bind() const override;
    void unbind() const override;

    void setBoolean(const std::string& name, bool value) override;
    void setFloat(const std::string& name, float value) override;
    void setFloat3(const std::string& name, const glm::vec3& value) override;
    void setFloat4(const std::string& name, const glm::vec4& value) override;
    void setInteger(const std::string& name, int value) override;
    void setMat4(const std::string& name, const glm::mat4& value) override;

    uint32_t getId() const {
        return program;
    };

   private:
    mutable absl::flat_hash_map<std::string, GLint> uniformLocations;

    static uint32_t compileShader(GLenum type, const std::string& source);
    uint32_t linkProgram(uint32_t vs, uint32_t fs);
    uint32_t linkProgram(uint32_t vs, uint32_t fs, uint32_t gs);

    uint32_t program = 0;

    GLint getUniformLocation(const std::string& name) const;
};

}  // namespace sponge::platform::opengl
