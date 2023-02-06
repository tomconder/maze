#include "openglresourcemanager.h"

#include <cassert>
#include <sstream>
#ifdef EMSCRIPTEN
#include <utility>
#endif

#include <SDL.h>

#include "core/log.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

std::unordered_map<std::string, std::shared_ptr<OpenGLFont>> OpenGLResourceManager::fonts;
std::unordered_map<std::string, std::shared_ptr<OpenGLModel>> OpenGLResourceManager::meshes;
std::unordered_map<std::string, std::shared_ptr<OpenGLShader>> OpenGLResourceManager::shaders;
std::unordered_map<std::string, std::shared_ptr<OpenGLTexture>> OpenGLResourceManager::textures;

std::shared_ptr<OpenGLFont> OpenGLResourceManager::getFont(const std::string &name) {
    assert(!name.empty());
    return fonts.at(name);
}

std::shared_ptr<OpenGLFont> OpenGLResourceManager::loadFont(const std::string &path, const std::string &name,
                                                            int screenWidth, int screenHeight) {
    assert(!path.empty());
    assert(!name.empty());

    if (fonts.find(name) != fonts.end()) {
        return fonts[name];
    }

    std::shared_ptr<OpenGLFont> font = loadFontFromFile(path, screenWidth, screenHeight);
    fonts[name] = font;

    return font;
}

std::shared_ptr<OpenGLModel> OpenGLResourceManager::getMesh(const std::string &name) {
    assert(!name.empty());
    return meshes.at(name);
}

std::shared_ptr<OpenGLModel> OpenGLResourceManager::loadMesh(const std::string &path, const std::string &name) {
    assert(!path.empty());
    assert(!name.empty());

    if (meshes.find(name) != meshes.end()) {
        return meshes[name];
    }

    std::shared_ptr<OpenGLModel> mesh = loadMeshFromFile(path);
    meshes[name] = mesh;

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

    std::string vertexSource = loadSourceFromFile(vertexShader);
    std::string fragmentSource = loadSourceFromFile(fragmentShader);

    auto shader = std::make_shared<OpenGLShader>(name, vertexSource, fragmentSource);

    shaders[shader->getName()] = shader;
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

    std::shared_ptr<OpenGLTexture> texture = loadTextureFromFile(path);
    textures[name] = texture;

    return texture;
}

std::shared_ptr<OpenGLTexture> OpenGLResourceManager::loadTextureWithType(const std::string &path,
                                                                          const std::string &typeName) {
    assert(!path.empty());
    assert(!typeName.empty());

    if (textures.find(path) != textures.end()) {
        return textures[path];
    }

    std::shared_ptr<OpenGLTexture> texture = loadTextureFromFile(path);
    texture->setType(typeName);
    textures[path] = texture;

    return texture;
}

std::shared_ptr<OpenGLFont> OpenGLResourceManager::loadFontFromFile(const std::string &path, int screenWidth,
                                                                    int screenHeight) {
    assert(!path.empty());

    SPONGE_CORE_INFO("Loading font file: {0}", path);

    auto font = std::make_shared<OpenGLFont>(screenWidth, screenHeight);
    font->load(path);

    return font;
}

std::shared_ptr<OpenGLModel> OpenGLResourceManager::loadMeshFromFile(const std::string &path) {
    assert(!path.empty());

    SPONGE_CORE_INFO("Loading mesh file: {0}", path);

    auto mesh = std::make_shared<OpenGLModel>();
    mesh->load(path);

    return mesh;
}

std::string OpenGLResourceManager::loadSourceFromFile(const std::string &path) {
    assert(!path.empty());

    SPONGE_CORE_INFO("Loading shader file: {0}", path);

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

    auto name = path;
    std::replace(name.begin(), name.end(), '\\', '/');

    SPONGE_CORE_INFO("Loading texture file: {0}", name);

    int bytesPerPixel;
    int height;
    int width;

    void *data = stbi_load(name.c_str(), &width, &height, &bytesPerPixel, 0);
    if (data == nullptr) {
        SPONGE_CORE_ERROR("Unable to open path: {}", path);
        return std::make_shared<OpenGLTexture>();
    }

    int pitch = (width * bytesPerPixel + 3) & ~3;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    Uint32 Rmask = 0x000000FF;
    Uint32 Gmask = 0x0000FF00;
    Uint32 Bmask = 0x00FF0000;
    Uint32 Amask = (bytesPerPixel == 4) ? 0xFF000000 : 0;
#else
    Uint32 s = (bytesPerPixel == 4) ? 0 : 8;
    Uint32 Rmask = 0xFF000000 >> s;
    Uint32 Gmask = 0x00FF0000 >> s;
    Uint32 Bmask = 0x0000FF00 >> s;
    Uint32 Amask = 0x000000FF >> s;
#endif

    SDL_Surface *surface =
        SDL_CreateRGBSurfaceFrom(data, width, height, bytesPerPixel * 8, pitch, Rmask, Gmask, Bmask, Amask);
    if (surface == nullptr) {
        SPONGE_CORE_ERROR("Unable to load texture file: {0}", path);
        return nullptr;
    }

    auto texture = std::make_shared<OpenGLTexture>();
    texture->generate(surface->w, surface->h, surface->format->BytesPerPixel,
                      static_cast<unsigned char *>(surface->pixels));

    SDL_FreeSurface(surface);

    return texture;
}
