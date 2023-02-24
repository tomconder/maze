#include "openglresourcemanager.h"

#include <cassert>
#include <sstream>
#ifdef EMSCRIPTEN
#include <utility>
#endif

#include <SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Sponge {

std::unordered_map<std::string, std::shared_ptr<OpenGLFont>> OpenGLResourceManager::fonts;
std::unordered_map<std::string, std::shared_ptr<OpenGLModel>> OpenGLResourceManager::models;
std::unordered_map<std::string, std::shared_ptr<OpenGLShader>> OpenGLResourceManager::shaders;
std::unordered_map<std::string, std::shared_ptr<OpenGLTexture>> OpenGLResourceManager::textures;

std::shared_ptr<OpenGLFont> OpenGLResourceManager::getFont(const std::string &name) {
    assert(!name.empty());
    return fonts.at(name);
}

std::shared_ptr<OpenGLFont> OpenGLResourceManager::loadFont(const std::string &path, const std::string &name) {
    assert(!path.empty());
    assert(!name.empty());

    if (fonts.find(name) != fonts.end()) {
        return fonts[name];
    }

    SPONGE_CORE_INFO("Loading font file: {}", path);

    std::shared_ptr<OpenGLFont> font = loadFontFromFile(path);
    fonts[name] = font;

    return font;
}

std::shared_ptr<OpenGLModel> OpenGLResourceManager::getModel(const std::string &name) {
    assert(!name.empty());
    return models.at(name);
}

std::shared_ptr<OpenGLModel> OpenGLResourceManager::loadModel(const std::string &path, const std::string &name) {
    assert(!path.empty());
    assert(!name.empty());

    if (models.find(name) != models.end()) {
        return models[name];
    }

    SPONGE_CORE_INFO("Loading mesh file: {}", path);

    std::shared_ptr<OpenGLModel> mesh = loadModelFromFile(path);
    models[name] = mesh;

    return mesh;
}

std::shared_ptr<OpenGLShader> OpenGLResourceManager::getShader(const std::string &name) {
    assert(!name.empty());
    return shaders.at(name);
}

std::shared_ptr<OpenGLShader> OpenGLResourceManager::loadShader(const std::string &vertexShader,
                                                                const std::string &fragmentShader,
                                                                const std::string &name) {
    assert(!vertexShader.empty());
    assert(!fragmentShader.empty());
    assert(!name.empty());

    if (shaders.find(name) != shaders.end()) {
        return shaders[name];
    }

    SPONGE_CORE_INFO("Loading vertex shader file: {}", vertexShader);
    std::string vertexSource = loadSourceFromFile(vertexShader);

    SPONGE_CORE_INFO("Loading fragment shader file: {}", fragmentShader);
    std::string fragmentSource = loadSourceFromFile(fragmentShader);

    auto shader = std::make_shared<OpenGLShader>(name, vertexSource, fragmentSource);

    shaders[name] = shader;
    return shader;
}

std::shared_ptr<OpenGLTexture> OpenGLResourceManager::getTexture(const std::string &name) {
    assert(!name.empty());
    return textures.at(name);
}

std::shared_ptr<OpenGLTexture> OpenGLResourceManager::loadTexture(const std::string &path, const std::string &name) {
    assert(!path.empty());
    assert(!name.empty());

    if (textures.find(name) != textures.end()) {
        return textures[name];
    }

    SPONGE_CORE_INFO("Loading texture file: {}", name);

    std::shared_ptr<OpenGLTexture> texture = loadTextureFromFile(path);
    textures[name] = texture;

    return texture;
}

std::shared_ptr<OpenGLFont> OpenGLResourceManager::loadFontFromFile(const std::string &path) {
    assert(!path.empty());

    auto font = std::make_shared<OpenGLFont>();
    font->load(path);

    return font;
}

std::shared_ptr<OpenGLModel> OpenGLResourceManager::loadModelFromFile(const std::string &path) {
    assert(!path.empty());

    auto mesh = std::make_shared<OpenGLModel>();
    mesh->load(path);

    return mesh;
}

std::string OpenGLResourceManager::loadSourceFromFile(const std::string &path) {
    assert(!path.empty());

    std::string code;
    if (std::ifstream stream(path, std::ios::in | std::ios::binary); stream.is_open()) {
        std::stringstream sstr;
        sstr << stream.rdbuf();
        code = sstr.str();
        stream.close();
    } else {
        SPONGE_CORE_ERROR("Unable to open file: {0}", path);
    }

    return code;
}

std::shared_ptr<OpenGLTexture> OpenGLResourceManager::loadTextureFromFile(const std::string &path) {
    assert(!path.empty());

    auto texture = std::make_shared<OpenGLTexture>();

    auto name = path;
    std::replace(name.begin(), name.end(), '\\', '/');

    int origFormat;
    int depth = 32;
    int height;
    int width;
    int channels = STBI_rgb_alpha;

    void *data = stbi_load(name.c_str(), &width, &height, &origFormat, channels);
    if (data == nullptr) {
        SPONGE_CORE_ERROR("Unable to load texture, path = {}: {}", path, stbi_failure_reason());
        return texture;
    }

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    Uint32 rmask = 0x000000FF;
    Uint32 gmask = 0x0000FF00;
    Uint32 bmask = 0x00FF0000;
    Uint32 amask = 0xFF000000;
#else
    Uint32 rmask = 0xFF000000;
    Uint32 gmask = 0x00FF0000;
    Uint32 bmask = 0x0000FF00;
    Uint32 amask = 0x000000FF;
#endif

    SDL_Surface *surface =
        SDL_CreateRGBSurfaceFrom(data, width, height, depth, channels * width, rmask, gmask, bmask, amask);
    if (surface == nullptr) {
        SPONGE_CORE_ERROR("Unable to load texture file: {0}", path);
        stbi_image_free(data);
        return texture;
    }

    texture->generate(surface->w, surface->h, surface->format->BytesPerPixel,
                      static_cast<unsigned char *>(surface->pixels));

    SDL_FreeSurface(surface);
    stbi_image_free(data);

    return texture;
}

}  // namespace Sponge
