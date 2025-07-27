#pragma once

#include "scene/gamecamera.hpp"
#include "scene/orthocamera.hpp"
#include "sponge.hpp"
#include <string>

namespace game {
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

   private:
    std::unordered_map<std::string, std::shared_ptr<T>> resources;
};

class ResourceManager {
   public:
    static std::shared_ptr<scene::OrthoCamera> createOrthoCamera(
        const scene::OrthoCameraCreateInfo& createInfo);

    static std::shared_ptr<scene::OrthoCamera> getOrthoCamera(
        const std::string& name);

    static std::shared_ptr<scene::GameCamera> createGameCamera(
        const scene::GameCameraCreateInfo& creatInfo);

    static std::shared_ptr<scene::GameCamera> getGameCamera(
        const std::string& name);

   private:
    ResourceManager() = default;

    static ResourceHandler<scene::OrthoCamera, scene::OrthoCameraCreateInfo>
        orthoCameraHandler;
    static ResourceHandler<scene::GameCamera, scene::GameCameraCreateInfo>
        gameCameraHandler;
};
}  // namespace game
