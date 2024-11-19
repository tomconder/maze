#include "resourcemanager.hpp"
#include "logging/log.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "core/file.hpp"
#include "stb_image.h"

namespace sponge::platform::opengl::renderer {

std::unordered_map<std::string, std::shared_ptr<scene::Font>>
    ResourceManager::fonts;
std::unordered_map<std::string, std::shared_ptr<scene::Model>>
    ResourceManager::models;
std::unordered_map<std::string, std::shared_ptr<Shader>>
    ResourceManager::shaders;
std::unordered_map<std::string, std::shared_ptr<Texture>>
    ResourceManager::textures;
std::string ResourceManager::assetsFolder = core::File::getResourceDir();

std::shared_ptr<scene::Font> ResourceManager::getFont(const std::string& name) {
    assert(!name.empty());
    return fonts.at(name);
}

std::shared_ptr<scene::Font> ResourceManager::loadFont(
    const std::string& name, const std::string& path) {
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

std::shared_ptr<scene::Model> ResourceManager::getModel(
    const std::string& name) {
    assert(!name.empty());
    return models.at(name);
}

std::shared_ptr<scene::Model> ResourceManager::loadModel(
    const std::string& name, const std::string& path) {
    assert(!path.empty());
    assert(!name.empty());

    if (models.contains(name)) {
        return models[name];
    }

    SPONGE_CORE_INFO("Loading model file: [{}, {}]", name, path);

    auto mesh = loadModelFromFile(assetsFolder + path);
    models[name] = mesh;

    return mesh;
}

std::shared_ptr<Shader> ResourceManager::getShader(const std::string& name) {
    assert(!name.empty());
    return shaders.at(name);
}

std::shared_ptr<Shader> ResourceManager::loadShader(
    const std::string& name, const std::string& vertexShader,
    const std::string& fragmentShader,
    const std::optional<std::string>& geometryShader) {
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

    std::shared_ptr<Shader> shader;
    if (geometryShader) {
        SPONGE_CORE_INFO("Loading geometry shader file: [{}, {}]", name,
                         *geometryShader);
        std::string geometrySource =
            loadSourceFromFile(assetsFolder + *geometryShader);

        assert(!geometrySource.empty());

        shader = std::make_shared<Shader>(vertexSource, fragmentSource,
                                          geometrySource);
    } else {
        shader = std::make_shared<Shader>(vertexSource, fragmentSource);
    }

    SPONGE_CORE_INFO("Created shader: [{}, {}]", name, shader->getId());

    shaders[name] = shader;
    return shader;
}

std::shared_ptr<Texture> ResourceManager::getTexture(const std::string& name) {
    assert(!name.empty());
    return textures.at(name);
}

std::shared_ptr<Texture> ResourceManager::loadTexture(const std::string& name,
                                                      const std::string& path,
                                                      const uint8_t flag) {
    assert(!path.empty());
    assert(!name.empty());

    if (textures.contains(name)) {
        return textures[name];
    }

    SPONGE_CORE_INFO(
        "Loading texture file: [{}, {}{}]", name, path,
        ((flag & GammaCorrection) == GammaCorrection) ? ", gamma" : "");

    std::string texturePath;
    if ((flag & ExcludeAssetsFolder) == ExcludeAssetsFolder) {
        texturePath = path;
    } else {
        texturePath = assetsFolder + path;
    }

    auto texture = loadTextureFromFile(
        texturePath, (flag & GammaCorrection) == GammaCorrection);
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

std::shared_ptr<scene::Model> ResourceManager::loadModelFromFile(
    const std::string& path) {
    assert(!path.empty());

    auto mesh = std::make_shared<scene::Model>();
    mesh->load(path);

    return mesh;
}

std::string ResourceManager::loadSourceFromFile(const std::string& path) {
    assert(!path.empty());

    std::string code;
    if (std::ifstream file(path, std::ios::in | std::ios::binary);
        file.good()) {
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
    const std::string& path, const bool gammaCorrection) {
    assert(!path.empty());

    auto texture = std::make_shared<Texture>();

    const std::filesystem::path name{ path };

    int bytesPerPixel;
    int height;
    int width;

    void* data =
        stbi_load(name.string().data(), &width, &height, &bytesPerPixel, 0);
    if (data == nullptr) {
        SPONGE_CORE_ERROR("Unable to load texture, path = {}: {}",
                          name.string(), stbi_failure_reason());
        return texture;
    }

    texture->generate(width, height, bytesPerPixel,
                      static_cast<const uint8_t*>(data), gammaCorrection);

    stbi_image_free(data);

    return texture;
}

}  // namespace sponge::platform::opengl::renderer
