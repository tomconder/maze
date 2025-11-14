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
                       sponge::core::TransparentStringHash,
                       sponge::core::TransparentStringEqual>
        resources;
};

// RESOURCE_MANAGER_FUNCS macro defined in sponge

class ResourceManager {
public:
    RESOURCE_MANAGER_FUNCS(OrthoCamera, scene::OrthoCamera,
                           scene::OrthoCameraCreateInfo, orthoCameraHandler);

    RESOURCE_MANAGER_FUNCS(GameCamera, scene::GameCamera,
                           scene::GameCameraCreateInfo, gameCameraHandler);

private:
    ResourceManager() = default;

    static ResourceHandler<scene::OrthoCamera, scene::OrthoCameraCreateInfo>
        orthoCameraHandler;
    static ResourceHandler<scene::GameCamera, scene::GameCameraCreateInfo>
        gameCameraHandler;
};
}  // namespace game
