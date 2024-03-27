#include "platform/opengl/openglresourcemanager.hpp"
#include "core/log.hpp"
#include <SDL.h>
#include <cassert>

#define STB_IMAGE_IMPLEMENTATION
#include "core/file.hpp"
#include "stb_image.h"

namespace sponge::renderer {

absl::flat_hash_map<std::string, std::shared_ptr<OpenGLFont>>
    OpenGLResourceManager::fonts;
absl::flat_hash_map<std::string, std::shared_ptr<OpenGLModel>>
    OpenGLResourceManager::models;
absl::flat_hash_map<std::string, std::shared_ptr<OpenGLShader>>
    OpenGLResourceManager::shaders;
absl::flat_hash_map<std::string, std::shared_ptr<OpenGLTexture>>
    OpenGLResourceManager::textures;
std::string OpenGLResourceManager::assetsFolder = File::getResourceDir();

std::shared_ptr<OpenGLFont> OpenGLResourceManager::getFont(
    const std::string& name) {
    assert(!name.empty());
    return fonts.at(name);
}

std::shared_ptr<OpenGLFont> OpenGLResourceManager::loadFont(
    const std::string& path, const std::string& name) {
    assert(!path.empty());
    assert(!name.empty());

    if (fonts.find(name) != fonts.end()) {
        return fonts[name];
    }

    SPONGE_CORE_INFO("Loading font file: {}", name);

    auto font = loadFontFromFile(assetsFolder + path);
    fonts[name] = font;

    return font;
}

std::shared_ptr<OpenGLModel> OpenGLResourceManager::getModel(
    const std::string& name) {
    assert(!name.empty());
    return models.at(name);
}

std::shared_ptr<OpenGLModel> OpenGLResourceManager::loadModel(
    const std::string& shaderName, const std::string& path,
    const std::string& name) {
    assert(!path.empty());
    assert(!name.empty());

    if (models.find(name) != models.end()) {
        return models[name];
    }

    SPONGE_CORE_INFO("Loading model file: {}", name);

    auto mesh = loadModelFromFile(shaderName, assetsFolder + path);
    models[name] = mesh;

    return mesh;
}

std::shared_ptr<OpenGLShader> OpenGLResourceManager::getShader(
    const std::string& name) {
    assert(!name.empty());
    return shaders.at(name);
}

std::shared_ptr<OpenGLShader> OpenGLResourceManager::loadShader(
    const std::string& vertexShader, const std::string& fragmentShader,
    const std::string& name) {
    assert(!vertexShader.empty());
    assert(!fragmentShader.empty());
    assert(!name.empty());

    if (shaders.find(name) != shaders.end()) {
        return shaders[name];
    }

    SPONGE_CORE_INFO("Loading vertex shader file: [{}, {}]", name,
                     vertexShader);
    std::string vertexSource = loadSourceFromFile(assetsFolder + vertexShader);

    assert(!vertexSource.empty());

    SPONGE_CORE_INFO("Loading fragment shader file: [{}, {}]", name,
                     fragmentShader);
    std::string fragmentSource =
        loadSourceFromFile(assetsFolder + fragmentShader);

    assert(!fragmentSource.empty());

    auto shader = std::make_shared<OpenGLShader>(vertexSource, fragmentSource);

    SPONGE_CORE_INFO("Created shader with id: [{}, {}]", name, shader->getId());

    shaders[name] = shader;
    return shader;
}

std::shared_ptr<OpenGLShader> OpenGLResourceManager::loadShader(
    const std::string& vertexShader, const std::string& fragmentShader,
    const std::string& geometryShader, const std::string& name) {
    assert(!vertexShader.empty());
    assert(!fragmentShader.empty());
    assert(!geometryShader.empty());
    assert(!name.empty());

    if (shaders.find(name) != shaders.end()) {
        return shaders[name];
    }

    SPONGE_CORE_INFO("Loading vertex shader file: [{}, {}]", name,
                     vertexShader);
    std::string vertexSource = loadSourceFromFile(assetsFolder + vertexShader);

    assert(!vertexSource.empty());

    SPONGE_CORE_INFO("Loading fragment shader file: [{}, {}]", name,
                     fragmentShader);
    std::string fragmentSource =
        loadSourceFromFile(assetsFolder + fragmentShader);

    assert(!fragmentSource.empty());

    SPONGE_CORE_INFO("Loading geometry shader file: [{}, {}]", name,
                     geometryShader);
    std::string geometrySource =
        loadSourceFromFile(assetsFolder + geometryShader);

    assert(!geometrySource.empty());

    auto shader = std::make_shared<OpenGLShader>(vertexSource, fragmentSource,
                                                 geometrySource);

    SPONGE_CORE_INFO("Created shader with id: [{}, {}]", name, shader->getId());

    shaders[name] = shader;
    return shader;
}

std::shared_ptr<OpenGLTexture> OpenGLResourceManager::getTexture(
    const std::string& name) {
    assert(!name.empty());
    return textures.at(name);
}

std::shared_ptr<OpenGLTexture> OpenGLResourceManager::loadTexture(
    const std::string& path, const std::string& name, const LoadFlag flag) {
    assert(!path.empty());
    assert(!name.empty());

    if (textures.find(name) != textures.end()) {
        return textures[name];
    }

    SPONGE_CORE_INFO("Loading texture file: {}", name);

    std::string texturePath;
    if (flag == ExcludeAssetsFolder) {
        texturePath = path;
    } else {
        texturePath = assetsFolder + path;
    }

    auto texture = loadTextureFromFile(texturePath);
    textures[name] = texture;

    return texture;
}

std::shared_ptr<OpenGLFont> OpenGLResourceManager::loadFontFromFile(
    const std::string& path) {
    assert(!path.empty());

    auto font = std::make_shared<OpenGLFont>();
    font->load(path);

    return font;
}

std::shared_ptr<OpenGLModel> OpenGLResourceManager::loadModelFromFile(
    const std::string& shaderName, const std::string& path) {
    assert(!path.empty());

    auto mesh = std::make_shared<OpenGLModel>();
    mesh->load(shaderName, path);

    return mesh;
}

std::string OpenGLResourceManager::loadSourceFromFile(const std::string& path) {
    assert(!path.empty());

    std::string code;
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (file.good()) {
        file.seekg(0, std::ios::end);
        code.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(code.data(), code.size());
        file.close();
    } else {
        SPONGE_CORE_ERROR("Unable to open file: {}", path);
    }

    return code;
}

std::shared_ptr<OpenGLTexture> OpenGLResourceManager::loadTextureFromFile(
    const std::string& path) {
    assert(!path.empty());

    auto texture = std::make_shared<OpenGLTexture>();

    std::filesystem::path name{ path };

    int origFormat;
    int depth = 32;
    int height;
    int width;
    int channels = STBI_rgb_alpha;

    void* data =
        stbi_load(name.string().data(), &width, &height, &origFormat, channels);
    if (data == nullptr) {
        SPONGE_CORE_ERROR("Unable to load texture, path = {}: {}",
                          name.string(), stbi_failure_reason());
        return texture;
    }

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    constexpr uint32_t rmask = 0x000000FF;
    constexpr uint32_t gmask = 0x0000FF00;
    constexpr uint32_t bmask = 0x00FF0000;
    constexpr uint32_t amask = 0xFF000000;
#else
    constexpr uint32_t rmask = 0xFF000000;
    constexpr uint32_t gmask = 0x00FF0000;
    constexpr uint32_t bmask = 0x0000FF00;
    constexpr uint32_t amask = 0x000000FF;
#endif

    SDL_Surface* surface =
        SDL_CreateRGBSurfaceFrom(data, width, height, depth, channels * width,
                                 rmask, gmask, bmask, amask);
    if (surface == nullptr) {
        SPONGE_CORE_ERROR("Unable to load texture file: {0}", path);
        stbi_image_free(data);
        return texture;
    }

    texture->generate(surface->w, surface->h, surface->format->BytesPerPixel,
                      static_cast<uint8_t*>(surface->pixels));

    SDL_FreeSurface(surface);
    stbi_image_free(data);

    return texture;
}

}  // namespace sponge::renderer
