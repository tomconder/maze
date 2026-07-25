#pragma once

#include "platform/opengl/renderer/assetmanager.hpp"
#include "scene/gamecamera.hpp"
#include "scene/orthocamera.hpp"

namespace game {
using sponge::platform::opengl::renderer::AssetHandler;

class ResourceManager {
public:
    ASSET_MANAGER_FUNCS(OrthoCamera, scene::OrthoCamera,
                        scene::OrthoCameraCreateInfo, orthoCameraHandler);

    ASSET_MANAGER_FUNCS(GameCamera, scene::GameCamera,
                        scene::GameCameraCreateInfo, gameCameraHandler);

private:
    ResourceManager() = default;

    static AssetHandler<scene::OrthoCamera, scene::OrthoCameraCreateInfo>
        orthoCameraHandler;
    static AssetHandler<scene::GameCamera, scene::GameCameraCreateInfo>
        gameCameraHandler;
};
}  // namespace game
