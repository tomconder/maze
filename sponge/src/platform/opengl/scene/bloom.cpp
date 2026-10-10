#include "platform/opengl/scene/bloom.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

Bloom::Bloom(const uint32_t width, const uint32_t height) :
    width(width), height(height) {
    initialize();
}

Bloom::~Bloom() {
    destroyTextures();
}

void Bloom::initialize() {
    auto makeShader = [](std::string_view name, const char* comp) {
        return AssetManager::createShader(renderer::ShaderCreateInfo{
            .name          = std::string(name),
            .computeShader = comp,
        });
    };

    extractShader = makeShader(extractShaderName, "bloom_extract.comp");
    downShader    = makeShader(downShaderName, "bloom_down.comp");
    upShader      = makeShader(upShaderName, "bloom_up.comp");

    createTextures();
}

void Bloom::createTextures() {
    auto makeMipTex = [](uint32_t w, uint32_t h) {
        return renderer::createRenderTarget(w, h, GL_RGBA16F, GL_LINEAR);
    };

    for (int i = 0; i < numLevels; i++) {
        const auto w = width >> (i + 1);
        const auto h = height >> (i + 1);

        downTextures[i] = makeMipTex(w, h);
        upTextures[i]   = makeMipTex(w, h);
    }
}

void Bloom::destroyTextures() {
    glDeleteTextures(numLevels, downTextures.data());
    glDeleteTextures(numLevels, upTextures.data());
    downTextures.fill(0);
    upTextures.fill(0);
}

void Bloom::process(const uint32_t sceneTexId, const float threshold) const {
    // Each pass samples what the one before it stored through an image.
    auto pass = [this](const renderer::Shader& shader, const uint32_t target,
                       const int level) {
        const auto w = std::max(width >> (level + 1), 1U);
        const auto h = std::max(height >> (level + 1), 1U);
        shader.setFloat2("targetSize", glm::vec2(static_cast<float>(w),
                                                 static_cast<float>(h)));
        glBindImageTexture(2, target, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                           GL_RGBA16F);
        shader.dispatch((w + 7) / 8, (h + 7) / 8);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    };

    // Extract bright pixels → down[0] at (w/2, h/2)
    extractShader->setFloat("threshold", threshold);
    glBindTextureUnit(0, sceneTexId);
    pass(*extractShader, downTextures[0], 0);

    // Downsample: down[i-1] → down[i]
    downShader->setFloat("offset", 1.0F);
    for (int i = 1; i < numLevels; i++) {
        glBindTextureUnit(0, downTextures[i - 1]);
        pass(*downShader, downTextures[i], i);
    }

    // Upsample: from deepest down level back up to up[0], accumulating each
    // level's downsample so the coarsest mip's texel structure never shows.
    upShader->setFloat("offset", 1.0F);
    for (int i = numLevels - 1; i >= 0; i--) {
        upShader->setFloat("accumulate", i == numLevels - 1 ? 0.F : 1.F);
        glBindTextureUnit(1, downTextures[i]);
        const uint32_t src =
            (i == numLevels - 1) ? downTextures[i] : upTextures[i + 1];
        glBindTextureUnit(0, src);
        pass(*upShader, upTextures[i], i);
    }
}

void Bloom::resize(const uint32_t newWidth, const uint32_t newHeight) {
    if (width == newWidth && height == newHeight) {
        return;
    }
    width  = newWidth;
    height = newHeight;
    destroyTextures();
    createTextures();
}

}  // namespace sponge::platform::opengl::scene
