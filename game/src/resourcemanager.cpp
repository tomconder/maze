#include "resourcemanager.hpp"

namespace game {
ResourceHandler<scene::OrthoCamera, scene::OrthoCameraCreateInfo>
    ResourceManager::orthoCameraHandler;
ResourceHandler<scene::GameCamera, scene::GameCameraCreateInfo>
    ResourceManager::gameCameraHandler;
}  // namespace game
