#pragma once

#include "scene/gamecamera.hpp"
#include "scene/orthocamera.hpp"
#include "sponge.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace game {
template <typename T, typename C>
class ResourceHandler final {
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
                       sponge::core::TransparentStringHash,
                       sponge::core::TransparentStringEqual>
        assets;
};

class ResourceManager {
public:
    ASSET_MANAGER_FUNCS(OrthoCamera, scene::OrthoCamera,
                        scene::OrthoCameraCreateInfo, orthoCameraHandler);

    ASSET_MANAGER_FUNCS(GameCamera, scene::GameCamera,
                        scene::GameCameraCreateInfo, gameCameraHandler);

private:
    ResourceManager() = default;

    static ResourceHandler<scene::OrthoCamera, scene::OrthoCameraCreateInfo>
        orthoCameraHandler;
    static ResourceHandler<scene::GameCamera, scene::GameCameraCreateInfo>
        gameCameraHandler;
};
}  // namespace game
