#pragma once

#include "scene/gamecamera.hpp"
#include "sponge.hpp"
#include <string>

namespace game {

class ResourceManager {
   public:
    static std::shared_ptr<sponge::scene::OrthoCamera> createOrthoCamera(
        const std::string& name);

    static std::shared_ptr<sponge::scene::OrthoCamera> getOrthoCamera(
        const std::string& name);

    static std::shared_ptr<scene::GameCamera> createGameCamera(
        const std::string& name);

    static std::shared_ptr<scene::GameCamera> getGameCamera(
        const std::string& name);

   private:
    ResourceManager() = default;

    static std::unordered_map<std::string,
                              std::shared_ptr<sponge::scene::OrthoCamera>>
        orthoCameras;
    static std::unordered_map<std::string, std::shared_ptr<scene::GameCamera>>
        gameCameras;
};

}  // namespace game
