#pragma once

#include "core/base.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/scene/font.hpp"
#include "platform/opengl/scene/model.hpp"
#include <optional>
#include <string>
#include <unordered_map>

namespace sponge::platform::opengl::renderer {
enum LoadFlag : uint8_t {
    None = 0,
    ExcludeAssetsFolder = BIT(0),
    GammaCorrection = BIT(1)
};

class ResourceManager {
   public:
    static std::shared_ptr<scene::Font> getFont(const std::string& name);
    static std::shared_ptr<scene::Font> loadFont(const std::string& name,
                                                 const std::string& path);

    static auto getFonts() {
        return fonts;
    }

    static std::shared_ptr<scene::Model> getModel(const std::string& name);
    static std::shared_ptr<scene::Model> loadModel(const std::string& name,
                                                   const std::string& path);

    static auto getModels() {
        return models;
    }

    static std::shared_ptr<Shader> loadShader(
        const std::string& name, const std::string& vertexShader,
        const std::string& fragmentShader,
        const std::optional<std::string>& geometryShader = std::nullopt);
    static std::shared_ptr<Shader> getShader(const std::string& name);

    static auto getShaders() {
        return shaders;
    }

    static std::shared_ptr<Texture> getTexture(const std::string& name);
    static std::shared_ptr<Texture> loadTexture(const std::string& name,
                                                const std::string& path,
                                                uint8_t flag = 0);

    static auto getTextures() {
        return textures;
    }

    static void setAssetsFolder(const std::string& folder) {
        assetsFolder = folder;
    }

   private:
    ResourceManager() = default;

    static std::shared_ptr<scene::Font> loadFontFromFile(
        const std::string& path);
    static std::shared_ptr<scene::Model> loadModelFromFile(
        const std::string& path);
    static std::string loadSourceFromFile(const std::string& path);
    static std::shared_ptr<Texture> loadTextureFromFile(const std::string& path,
                                                        bool gammaCorrection);

    static std::string assetsFolder;

    static std::unordered_map<std::string, std::shared_ptr<scene::Font>> fonts;
    static std::unordered_map<std::string, std::shared_ptr<scene::Model>>
        models;
    static std::unordered_map<std::string, std::shared_ptr<Shader>> shaders;
    static std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
};
}  // namespace sponge::platform::opengl::renderer
