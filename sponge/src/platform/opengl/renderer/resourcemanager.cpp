#include "resourcemanager.hpp"

namespace sponge::platform::opengl::renderer {
ResourceHandler<scene::Font, scene::FontCreateInfo>
    ResourceManager::fontHandler;
ResourceHandler<Texture, TextureCreateInfo> ResourceManager::textureHandler;
ResourceHandler<scene::Model, scene::ModelCreateInfo>
    ResourceManager::modelHandler;
ResourceHandler<Shader, ShaderCreateInfo> ResourceManager::shaderHandler;

std::shared_ptr<scene::Font> ResourceManager::createFont(
    const scene::FontCreateInfo& fontCreateInfo) {
    return fontHandler.load(fontCreateInfo);
}

std::shared_ptr<scene::Font> ResourceManager::getFont(const std::string& name) {
    return fontHandler.get(name);
}

std::unordered_map<std::string, std::shared_ptr<scene::Font>>
ResourceManager::getFonts() {
    return fontHandler.getResources();
}

std::shared_ptr<scene::Model> ResourceManager::createModel(
    const scene::ModelCreateInfo& modelCreateInfo) {
    return modelHandler.load(modelCreateInfo);
}

std::shared_ptr<scene::Model> ResourceManager::getModel(
    const std::string& name) {
    return modelHandler.get(name);
}

std::unordered_map<std::string, std::shared_ptr<scene::Model>>
ResourceManager::getModels() {
    return modelHandler.getResources();
}

std::shared_ptr<Shader> ResourceManager::createShader(
    const ShaderCreateInfo& createInfo) {
    return shaderHandler.load(createInfo);
}

std::shared_ptr<Shader> ResourceManager::getShader(const std::string& name) {
    return shaderHandler.get(name);
}

std::unordered_map<std::string, std::shared_ptr<Shader>>
ResourceManager::getShaders() {
    return shaderHandler.getResources();
}

std::shared_ptr<Texture> ResourceManager::createTexture(
    const TextureCreateInfo& textureCreateInfo) {
    return textureHandler.load(textureCreateInfo);
}

std::shared_ptr<Texture> ResourceManager::getTexture(const std::string& name) {
    return textureHandler.get(name);
}

std::unordered_map<std::string, std::shared_ptr<Texture>>
ResourceManager::getTextures() {
    return textureHandler.getResources();
}
}  // namespace sponge::platform::opengl::renderer
