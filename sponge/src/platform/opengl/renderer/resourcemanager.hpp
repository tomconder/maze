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

#define RESOURCE_MANAGER_FUNCS(Name, Type, CreateInfoType, Handler)   \
    static std::shared_ptr<Type> create##Name(                        \
        const CreateInfoType& createInfo) {                           \
        return Handler.load(createInfo);                              \
    }                                                                 \
                                                                      \
    static std::shared_ptr<Type> get##Name(const std::string& name) { \
        return Handler.get(name);                                     \
    }                                                                 \
                                                                      \
    static std::unordered_map<std::string, std::shared_ptr<Type>>     \
    get##Name##s() {                                                  \
        return Handler.getResources();                                \
    }

class ResourceManager {
   public:
    RESOURCE_MANAGER_FUNCS(Font, scene::Font, scene::FontCreateInfo,
                           fontHandler);

    RESOURCE_MANAGER_FUNCS(Model, scene::Model, scene::ModelCreateInfo,
                           modelHandler);

    RESOURCE_MANAGER_FUNCS(Shader, Shader, ShaderCreateInfo, shaderHandler);

    RESOURCE_MANAGER_FUNCS(Texture, Texture, TextureCreateInfo, textureHandler);

   private:
    ResourceManager() = default;

    static ResourceHandler<Shader, ShaderCreateInfo> shaderHandler;
    static ResourceHandler<Texture, TextureCreateInfo> textureHandler;
    static ResourceHandler<scene::Font, scene::FontCreateInfo> fontHandler;
    static ResourceHandler<scene::Model, scene::ModelCreateInfo> modelHandler;
};
}  // namespace sponge::platform::opengl::renderer
