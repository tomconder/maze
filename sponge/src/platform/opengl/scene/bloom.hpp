#pragma once

#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string_view>

namespace sponge::platform::opengl::scene {

class Bloom {
public:
    Bloom() = delete;
    Bloom(uint32_t width, uint32_t height);
    ~Bloom();

    Bloom(const Bloom&)            = delete;
    Bloom& operator=(const Bloom&) = delete;

    void begin() const;
    void end() const;
    void process(float threshold) const;
    void apply(float intensity) const;

    uint32_t getSceneTexture() const {
        return sceneColorTexture;
    }

    uint32_t getBloomTexture() const {
        return upTextures[0];
    }

    void resize(uint32_t newWidth, uint32_t newHeight);

private:
    static constexpr std::string_view extractShaderName   = "bloom_extract";
    static constexpr std::string_view downShaderName      = "bloom_down";
    static constexpr std::string_view upShaderName        = "bloom_up";
    static constexpr std::string_view compositeShaderName = "bloom_composite";
    static constexpr int              numLevels           = 5;

    std::shared_ptr<renderer::Shader>       extractShader;
    std::shared_ptr<renderer::Shader>       downShader;
    std::shared_ptr<renderer::Shader>       upShader;
    std::shared_ptr<renderer::Shader>       compositeShader;
    std::unique_ptr<renderer::VertexArray>  vao;
    std::unique_ptr<renderer::VertexBuffer> vbo;

    uint32_t sceneFbo          = 0;
    uint32_t sceneColorTexture = 0;
    uint32_t sceneDepthRbo     = 0;

    std::array<uint32_t, numLevels> downFbos{};
    std::array<uint32_t, numLevels> downTextures{};
    std::array<uint32_t, numLevels> upFbos{};
    std::array<uint32_t, numLevels> upTextures{};

    uint32_t width  = 0;
    uint32_t height = 0;

    void initialize();
    void createFramebuffers();
    void destroyFramebuffers();
    void renderQuad() const;
};

}  // namespace sponge::platform::opengl::scene
