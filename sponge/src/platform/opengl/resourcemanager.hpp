#pragma once

#include "platform/opengl/font.hpp"
#include "platform/opengl/model.hpp"
#include "platform/opengl/shader.hpp"
#include "platform/opengl/texture.hpp"
#include <string>

namespace sponge::platform::opengl {

enum LoadFlag { ExcludeAssetsFolder, None };

class ResourceManager {
   public:
    static std::shared_ptr<Font> getFont(const std::string& name);
    static std::shared_ptr<Font> loadFont(const std::string& path,
                                          const std::string& name);

    static std::shared_ptr<Model> getModel(const std::string& name);
    static std::shared_ptr<Model> loadModel(const std::string& shaderName,
                                            const std::string& path,
                                            const std::string& name);

    static std::shared_ptr<Shader> loadShader(const std::string& vertexShader,
                                              const std::string& fragmentShader,
                                              const std::string& name);
    static std::shared_ptr<Shader> loadShader(const std::string& vertexShader,
                                              const std::string& fragmentShader,
                                              const std::string& geometryShader,
                                              const std::string& name);
    static std::shared_ptr<Shader> getShader(const std::string& name);

    static std::shared_ptr<Texture> getTexture(const std::string& name);
    static std::shared_ptr<Texture> loadTexture(const std::string& path,
                                                const std::string& name,
                                                LoadFlag flag = None);

    static void setAssetsFolder(const std::string& folder) {
        assetsFolder = folder;
    }

   private:
    ResourceManager() = default;

    static std::shared_ptr<Font> loadFontFromFile(const std::string& path);
    static std::shared_ptr<Model> loadModelFromFile(
        const std::string& shaderName, const std::string& path);
    static std::string loadSourceFromFile(const std::string& path);
    static std::shared_ptr<Texture> loadTextureFromFile(
        const std::string& path);

    static std::string assetsFolder;

    static std::unordered_map<std::string, std::shared_ptr<Font>> fonts;
    static std::unordered_map<std::string, std::shared_ptr<Model>> models;
    static std::unordered_map<std::string, std::shared_ptr<Shader>> shaders;
    static std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
};

}  // namespace sponge::platform::opengl
