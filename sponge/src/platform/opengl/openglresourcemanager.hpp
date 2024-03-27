#pragma once

#include "platform/opengl/openglfont.hpp"
#include "platform/opengl/openglmodel.hpp"
#include "platform/opengl/openglshader.hpp"
#include "platform/opengl/opengltexture.hpp"
#include <absl/container/flat_hash_map.h>
#include <string>

namespace sponge::renderer {

enum LoadFlag { ExcludeAssetsFolder, None };

class OpenGLResourceManager {
   public:
    static std::shared_ptr<OpenGLFont> getFont(const std::string& name);
    static std::shared_ptr<OpenGLFont> loadFont(const std::string& path,
                                                const std::string& name);

    static std::shared_ptr<OpenGLModel> getModel(const std::string& name);
    static std::shared_ptr<OpenGLModel> loadModel(const std::string& shaderName,
                                                  const std::string& path,
                                                  const std::string& name);

    static std::shared_ptr<OpenGLShader> loadShader(
        const std::string& vertexShader, const std::string& fragmentShader,
        const std::string& name);
    static std::shared_ptr<OpenGLShader> loadShader(
        const std::string& vertexShader, const std::string& fragmentShader,
        const std::string& geometryShader, const std::string& name);
    static std::shared_ptr<OpenGLShader> getShader(const std::string& name);

    static std::shared_ptr<OpenGLTexture> getTexture(const std::string& name);
    static std::shared_ptr<OpenGLTexture> loadTexture(const std::string& path,
                                                      const std::string& name,
                                                      LoadFlag flag = None);

    static void setAssetsFolder(const std::string& folder) {
        assetsFolder = folder;
    }

   private:
    OpenGLResourceManager() = default;

    static std::shared_ptr<OpenGLFont> loadFontFromFile(
        const std::string& path);
    static std::shared_ptr<OpenGLModel> loadModelFromFile(
        const std::string& shaderName, const std::string& path);
    static std::string loadSourceFromFile(const std::string& path);
    static std::shared_ptr<OpenGLTexture> loadTextureFromFile(
        const std::string& path);

    static std::string assetsFolder;

    static absl::flat_hash_map<std::string, std::shared_ptr<OpenGLFont>> fonts;
    static absl::flat_hash_map<std::string, std::shared_ptr<OpenGLModel>>
        models;
    static absl::flat_hash_map<std::string, std::shared_ptr<OpenGLShader>>
        shaders;
    static absl::flat_hash_map<std::string, std::shared_ptr<OpenGLTexture>>
        textures;
};

}  // namespace sponge::renderer
