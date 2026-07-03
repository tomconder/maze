// sponge/src/platform/opengl/renderer/computeshader.hpp
#pragma once

#include <cstdint>
#include <string>

namespace sponge::platform::opengl::renderer {

class ComputeShader final {
public:
    ComputeShader(const std::string& name, const std::string& glslPath);
    ~ComputeShader();

    ComputeShader(const ComputeShader&)            = delete;
    ComputeShader& operator=(const ComputeShader&) = delete;
    ComputeShader(ComputeShader&& other) noexcept;
    ComputeShader& operator=(ComputeShader&& other) noexcept;

    // Dispatch groupsX * groupsY * groupsZ work groups.
    // Caller must issue glMemoryBarrier after if SSBOs are read by later
    // stages.
    void dispatch(uint32_t groupsX, uint32_t groupsY = 1,
                  uint32_t groupsZ = 1) const;

    uint32_t getId() const {
        return program;
    }

private:
    uint32_t    program = 0;
    std::string shaderName;
};

}  // namespace sponge::platform::opengl::renderer
