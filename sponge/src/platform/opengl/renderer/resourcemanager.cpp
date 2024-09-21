#include "resourcemanager.hpp"
#include "logging/log.hpp"
#include <SDL.h>
#include <cassert>
#include <filesystem>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "core/file.hpp"
#include "stb_image.h"

namespace sponge::platform::opengl::renderer {

std::unordered_map<std::string, std::shared_ptr<scene::Font>>
    ResourceManager::fonts;
std::unordered_map<std::string, std::shared_ptr<Model>> ResourceManager::models;
std::unordered_map<std::string, std::shared_ptr<Shader>>
    ResourceManager::shaders;
std::unordered_map<std::string, std::shared_ptr<Texture>>
    ResourceManager::textures;
std::string ResourceManager::assetsFolder = core::File::getResourceDir();

std::shared_ptr<scene::Font> ResourceManager::getFont(
    const std::string& name) {
    assert(!name.empty());
    return fonts.at(name);
}

std::shared_ptr<scene::Font> ResourceManager::loadFont(
    const std::string& path, const std::string& name) {
    assert(!path.empty());
    assert(!name.empty());

    if (fonts.contains(name)) {
        return fonts[name];
    }

    SPONGE_CORE_INFO("Loading font file: [{}, {}]", name, path);

    auto font = loadFontFromFile(assetsFolder + path);
    fonts[name] = font;

    return font;
}

std::shared_ptr<Model> ResourceManager::getModel(const std::string& name) {
    assert(!name.empty());
    return models.at(name);
}

std::shared_ptr<Model> ResourceManager::loadModel(const std::string& shaderName,
                                                  const std::string& path,
                                                  const std::string& name) {
    assert(!path.empty());
    assert(!name.empty());

    if (models.contains(name)) {
        return models[name];
    }

    SPONGE_CORE_INFO("Loading model file: [{}, {}]", name, path);

    auto mesh = loadModelFromFile(shaderName, assetsFolder + path);
    models[name] = mesh;

    return mesh;
}

std::shared_ptr<Shader> ResourceManager::getShader(const std::string& name) {
    assert(!name.empty());
    return shaders.at(name);
}

std::shared_ptr<Shader> ResourceManager::loadShader(
    const std::string& vertexShader, const std::string& fragmentShader,
    const std::string& name) {
    assert(!vertexShader.empty());
    assert(!fragmentShader.empty());
    assert(!name.empty());

    if (shaders.contains(name)) {
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

    auto shader = std::make_shared<Shader>(vertexSource, fragmentSource);

    SPONGE_CORE_INFO("Created shader with id: [{}, {}]", name, shader->getId());

    shaders[name] = shader;
    return shader;
}

std::shared_ptr<Shader> ResourceManager::loadShader(
    const std::string& vertexShader, const std::string& fragmentShader,
    const std::string& geometryShader, const std::string& name) {
    assert(!vertexShader.empty());
    assert(!fragmentShader.empty());
    assert(!geometryShader.empty());
    assert(!name.empty());

    if (shaders.contains(name)) {
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

    auto shader =
        std::make_shared<Shader>(vertexSource, fragmentSource, geometrySource);

    SPONGE_CORE_INFO("Created shader with id: [{}, {}]", name, shader->getId());

    shaders[name] = shader;
    return shader;
}

std::shared_ptr<Texture> ResourceManager::getTexture(const std::string& name) {
    assert(!name.empty());
    return textures.at(name);
}

std::shared_ptr<Texture> ResourceManager::loadTexture(const std::string& path,
                                                      const std::string& name,
                                                      const LoadFlag flag) {
    assert(!path.empty());
    assert(!name.empty());

    if (textures.contains(name)) {
        return textures[name];
    }

    SPONGE_CORE_INFO("Loading texture file: [{}, {}]", name, path);

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

std::shared_ptr<scene::Font> ResourceManager::loadFontFromFile(
    const std::string& path) {
    assert(!path.empty());

    auto font = std::make_shared<scene::Font>();
    font->load(path);
    font->log();

    return font;
}

std::shared_ptr<Model> ResourceManager::loadModelFromFile(
    const std::string& shaderName, const std::string& path) {
    assert(!path.empty());

    auto mesh = std::make_shared<Model>();
    mesh->load(shaderName, path);

    return mesh;
}

std::string ResourceManager::loadSourceFromFile(const std::string& path) {
    assert(!path.empty());

    std::string code;
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (file.good()) {
        file.seekg(0, std::ios::end);
        const auto size = file.tellg();
        file.seekg(0, std::ios::beg);
        code.resize(size);
        file.read(code.data(), size);
        file.close();
    } else {
        SPONGE_CORE_ERROR("Unable to open file: {}", path);
    }

    return code;
}

std::shared_ptr<Texture> ResourceManager::loadTextureFromFile(
    const std::string& path) {
    assert(!path.empty());

    auto texture = std::make_shared<Texture>();

    const std::filesystem::path name{ path };

    int origFormat;
    constexpr int depth = 32;
    int height;
    int width;
    constexpr int channels = STBI_rgb_alpha;

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

}  // namespace sponge::platform::opengl::renderer
