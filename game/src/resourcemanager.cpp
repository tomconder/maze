#include "resourcemanager.hpp"

namespace game {
AssetHandler<scene::OrthoCamera, scene::OrthoCameraCreateInfo>
    ResourceManager::orthoCameraHandler;
AssetHandler<scene::GameCamera, scene::GameCameraCreateInfo>
    ResourceManager::gameCameraHandler;
}  // namespace game
