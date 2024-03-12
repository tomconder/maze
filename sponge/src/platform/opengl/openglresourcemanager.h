#pragma once

#include "platform/opengl/openglfont.h"
#include "platform/opengl/openglmodel.h"
#include "platform/opengl/openglshader.h"
#include "platform/opengl/opengltexture.h"
#include <absl/container/flat_hash_map.h>
#include <string>

namespace sponge::renderer {

class OpenGLResourceManager {
   public:
    static std::shared_ptr<OpenGLFont> getFont(const std::string& name);
    static std::shared_ptr<OpenGLFont> loadFont(const std::string& path,
                                                const std::string& name);

    static std::shared_ptr<OpenGLModel> getModel(const std::string& name);
    static std::shared_ptr<OpenGLModel> loadModel(const std::string& path,
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
                                                      const std::string& name);

   private:
    OpenGLResourceManager() = default;

    static std::shared_ptr<OpenGLFont> loadFontFromFile(
        const std::string& path);
    static std::shared_ptr<OpenGLModel> loadModelFromFile(
        const std::string& path);
    static std::string loadSourceFromFile(const std::string& path);
    static std::shared_ptr<OpenGLTexture> loadTextureFromFile(
        const std::string& path);

    static absl::flat_hash_map<std::string, std::shared_ptr<OpenGLFont>> fonts;
    static absl::flat_hash_map<std::string, std::shared_ptr<OpenGLModel>>
        models;
    static absl::flat_hash_map<std::string, std::shared_ptr<OpenGLShader>>
        shaders;
    static absl::flat_hash_map<std::string, std::shared_ptr<OpenGLTexture>>
        textures;
};

}  // namespace sponge::renderer
