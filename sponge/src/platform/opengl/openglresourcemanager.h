#pragma once

#include <fstream>
#include <string>
#include <unordered_map>

#include "openglfont.h"
#include "openglmodel.h"
#include "openglshader.h"
#include "opengltexture.h"

class OpenGLResourceManager {
   public:
    static std::shared_ptr<OpenGLFont> getFont(const std::string &name);
    static std::shared_ptr<OpenGLFont> loadFont(const std::string &path, const std::string &name, int screenWidth,
                                                int screenHeight);

    static std::shared_ptr<OpenGLModel> getModel(const std::string &name);
    static std::shared_ptr<OpenGLModel> loadModel(const std::string &path, const std::string &name);

    static std::shared_ptr<OpenGLShader> loadShader(const std::string &vertexShader, const std::string &fragmentShader,
                                                    const std::string &name);
    static std::shared_ptr<OpenGLShader> getShader(const std::string &name);

    static std::shared_ptr<OpenGLTexture> getTexture(const std::string &name);
    static std::shared_ptr<OpenGLTexture> loadTexture(const std::string &path, const std::string &name);
    static std::shared_ptr<OpenGLTexture> loadTextureWithType(const std::string &path, const std::string &typeName);

   private:
    OpenGLResourceManager() = default;

    static std::shared_ptr<OpenGLFont> loadFontFromFile(const std::string &path, int screenWidth, int screenHeight);
    static std::shared_ptr<OpenGLModel> loadModelFromFile(const std::string &path);
    static std::string loadSourceFromFile(const std::string &path);
    static std::shared_ptr<OpenGLTexture> loadTextureFromFile(const std::string &path);

    static std::unordered_map<std::string, std::shared_ptr<OpenGLFont>> fonts;
    static std::unordered_map<std::string, std::shared_ptr<OpenGLModel>> models;
    static std::unordered_map<std::string, std::shared_ptr<OpenGLShader>> shaders;
    static std::unordered_map<std::string, std::shared_ptr<OpenGLTexture>> textures;
};
