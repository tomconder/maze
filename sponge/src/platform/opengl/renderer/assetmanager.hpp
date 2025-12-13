#pragma once

#include "core/stringutils.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/scene/model.hpp"
#include "platform/opengl/scene/msdffont.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

namespace sponge::platform::opengl::renderer {

// Asset cache with heterogeneous string_view lookup support
template <typename T, typename C>
class AssetHandler final {
public:
    std::shared_ptr<T> load(const C& createInfo) {
        assert(!createInfo.name.empty());

        if (assets.contains(createInfo.name)) {
            return assets[createInfo.name];
        }

        auto asset              = std::make_shared<T>(createInfo);
        assets[createInfo.name] = asset;

        return asset;
    }

    std::shared_ptr<T> get(std::string_view name) const {
        assert(!name.empty());
        if (const auto it = assets.find(name); it != assets.end()) {
            return it->second;
        }
        return nullptr;
    }

    const auto& getAssets() const {
        return assets;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<T>,
                       core::TransparentStringHash,
                       core::TransparentStringEqual>
        assets;
};

#define ASSET_MANAGER_FUNCS(Name, Type, CreateInfoType, Handler)    \
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
        return (Handler).getAssets();                               \
    }

class AssetManager {
public:
    ASSET_MANAGER_FUNCS(Font, scene::MSDFFont, scene::FontCreateInfo,
                        fontHandler);

    ASSET_MANAGER_FUNCS(Model, scene::Model, scene::ModelCreateInfo,
                        modelHandler);

    ASSET_MANAGER_FUNCS(Shader, Shader, ShaderCreateInfo, shaderHandler);

    ASSET_MANAGER_FUNCS(Texture, Texture, TextureCreateInfo, textureHandler);

private:
    AssetManager() = default;

    static AssetHandler<Shader, ShaderCreateInfo>               shaderHandler;
    static AssetHandler<Texture, TextureCreateInfo>             textureHandler;
    static AssetHandler<scene::MSDFFont, scene::FontCreateInfo> fontHandler;
    static AssetHandler<scene::Model, scene::ModelCreateInfo>   modelHandler;
};
}  // namespace sponge::platform::opengl::renderer
