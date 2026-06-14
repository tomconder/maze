#pragma once

#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/model.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

namespace sponge::platform::opengl::renderer {

// Asset cache. create*() returns a shared_ptr handle; callers retain it and
// reuse it directly — there is no by-name lookup at runtime.
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

    const auto& getAssets() const {
        return assets;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<T>> assets;
};

#define ASSET_MANAGER_FUNCS(Name, Type, CreateInfoType, Handler) \
    static std::shared_ptr<Type> create##Name(                   \
        const CreateInfoType& createInfo) {                      \
        return (Handler).load(createInfo);                       \
    }                                                            \
                                                                 \
    static auto& get##Name##s() {                                \
        return (Handler).getAssets();                            \
    }

class AssetManager {
public:
    ASSET_MANAGER_FUNCS(Font, scene::BitmapFont, scene::FontCreateInfo,
                        fontHandler);

    ASSET_MANAGER_FUNCS(Model, scene::Model, scene::ModelCreateInfo,
                        modelHandler);

    ASSET_MANAGER_FUNCS(Shader, Shader, ShaderCreateInfo, shaderHandler);

    ASSET_MANAGER_FUNCS(Texture, Texture, TextureCreateInfo, textureHandler);

private:
    AssetManager() = default;

    static AssetHandler<Shader, ShaderCreateInfo>   shaderHandler;
    static AssetHandler<Texture, TextureCreateInfo> textureHandler;
    static AssetHandler<scene::BitmapFont, scene::FontCreateInfo> fontHandler;
    static AssetHandler<scene::Model, scene::ModelCreateInfo>     modelHandler;
};
}  // namespace sponge::platform::opengl::renderer
