#pragma once

#include "core/stringutils.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/scene/font.hpp"
#include "platform/opengl/scene/model.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

namespace sponge::platform::opengl::renderer {

// Resource cache with heterogeneous string_view lookup support
template <typename T, typename C>
class ResourceHandler final {
public:
    std::shared_ptr<T> load(const C& createInfo) {
        assert(!createInfo.name.empty());

        if (resources.contains(createInfo.name)) {
            return resources[createInfo.name];
        }

        auto resource              = std::make_shared<T>(createInfo);
        resources[createInfo.name] = resource;

        return resource;
    }

    std::shared_ptr<T> get(std::string_view name) const {
        assert(!name.empty());
        if (const auto it = resources.find(name); it != resources.end()) {
            return it->second;
        }
        return nullptr;
    }

    const auto& getResources() const {
        return resources;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<T>,
                       core::TransparentStringHash,
                       core::TransparentStringEqual>
        resources;
};

#define RESOURCE_MANAGER_FUNCS(Name, Type, CreateInfoType, Handler) \
    static std::shared_ptr<Type> create##Name(                      \
        const CreateInfoType& createInfo) {                         \
        return (Handler).load(createInfo);                          \
    }                                                               \
                                                                    \
    static std::shared_ptr<Type> get##Name(std::string_view name) { \
        return (Handler).get(name);                                 \
    }                                                               \
                                                                    \
    static auto& get##Name##s() {                                   \
        return (Handler).getResources();                            \
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

    static ResourceHandler<Shader, ShaderCreateInfo>             shaderHandler;
    static ResourceHandler<Texture, TextureCreateInfo>           textureHandler;
    static ResourceHandler<scene::Font, scene::FontCreateInfo>   fontHandler;
    static ResourceHandler<scene::Model, scene::ModelCreateInfo> modelHandler;
};
}  // namespace sponge::platform::opengl::renderer
