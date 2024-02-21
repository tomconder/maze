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
    static std::shared_ptr<OpenGLShader> getShader(const std::string& name);

    static std::shared_ptr<OpenGLTexture> getTexture(const std::string& name);
    static std::shared_ptr<OpenGLTexture> loadTexture(const std::string& path,
                                                      const std::string& name);

    static std::shared_ptr<OpenGLVertexArray> createVertexArray(
        const std::string& id);
    static std::shared_ptr<OpenGLVertexArray> getVertexArray(
        const std::string& id);
    static std::shared_ptr<OpenGLBuffer> createBuffer(const std::string& id,
                                                      uint32_t size);
    static std::shared_ptr<OpenGLBuffer> createBuffer(const std::string& id,
                                                      const float* vertices,
                                                      uint32_t size);
    static std::shared_ptr<OpenGLBuffer> getBuffer(const std::string& id);
    static std::shared_ptr<OpenGLElementBuffer> createElementBuffer(
        const std::string& id, uint32_t size);
    static std::shared_ptr<OpenGLElementBuffer> createElementBuffer(
        const std::string& id, const uint32_t* indices, uint32_t size);
    static std::shared_ptr<OpenGLElementBuffer> getElementBuffer(
        const std::string& id);

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

    static absl::flat_hash_map<const std::string,
                               std::shared_ptr<OpenGLVertexArray>>
        vertexArrays;
    static absl::flat_hash_map<const std::string, std::shared_ptr<OpenGLBuffer>>
        buffers;
    static absl::flat_hash_map<const std::string,
                               std::shared_ptr<OpenGLElementBuffer>>
        elementBuffers;
};

}  // namespace sponge::renderer
