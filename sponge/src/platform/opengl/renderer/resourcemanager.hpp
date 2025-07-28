#pragma once

#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/scene/font.hpp"
#include "platform/opengl/scene/model.hpp"
#include <string>
#include <unordered_map>

namespace sponge::platform::opengl::renderer {
template <typename T, typename C>
class ResourceHandler final {
   public:
    std::shared_ptr<T> load(const C& createInfo) {
        assert(!createInfo.name.empty());

        if (resources.contains(createInfo.name)) {
            return resources[createInfo.name];
        }

        auto resource = std::make_shared<T>(createInfo);
        resources[createInfo.name] = resource;

        return resource;
    }

    std::shared_ptr<T> get(const std::string& name) const {
        assert(!name.empty());
        return resources.at(name);
    }

    std::unordered_map<std::string, std::shared_ptr<T>> getResources() const {
        return resources;
    }

   private:
    std::unordered_map<std::string, std::shared_ptr<T>> resources;
};

class ResourceManager {
   public:
    static std::shared_ptr<scene::Font> createFont(
        const scene::FontCreateInfo& fontCreateInfo);

    static std::shared_ptr<scene::Font> getFont(const std::string& name);

    static std::unordered_map<std::string, std::shared_ptr<scene::Font>>
    getFonts();

    static std::shared_ptr<scene::Model> createModel(
        const scene::ModelCreateInfo& modelCreateInfo);

    static std::shared_ptr<scene::Model> getModel(const std::string& name);

    static std::unordered_map<std::string, std::shared_ptr<scene::Model>>
    getModels();

    static std::shared_ptr<Shader> createShader(
        const ShaderCreateInfo& createInfo);

    static std::shared_ptr<Shader> getShader(const std::string& name);

    static std::unordered_map<std::string, std::shared_ptr<Shader>>
    getShaders();

    static std::shared_ptr<Texture> createTexture(
        const TextureCreateInfo& textureCreateInfo);

    static std::shared_ptr<Texture> getTexture(const std::string& name);

    static std::unordered_map<std::string, std::shared_ptr<Texture>>
    getTextures();

   private:
    ResourceManager() = default;

    static ResourceHandler<Shader, ShaderCreateInfo> shaderHandler;
    static ResourceHandler<Texture, TextureCreateInfo> textureHandler;
    static ResourceHandler<scene::Font, scene::FontCreateInfo> fontHandler;
    static ResourceHandler<scene::Model, scene::ModelCreateInfo> modelHandler;
};
}  // namespace sponge::platform::opengl::renderer
