#include "resourcemanager.hpp"

namespace game {
ResourceHandler<scene::OrthoCamera, scene::OrthoCameraCreateInfo>
    ResourceManager::orthoCameraHandler;
ResourceHandler<scene::GameCamera, scene::GameCameraCreateInfo>
    ResourceManager::gameCameraHandler;

std::shared_ptr<scene::OrthoCamera> ResourceManager::createOrthoCamera(
    const scene::OrthoCameraCreateInfo& createInfo) {
    return orthoCameraHandler.load(createInfo);
}

std::shared_ptr<scene::OrthoCamera> ResourceManager::getOrthoCamera(
    const std::string& name) {
    return ResourceManager::orthoCameraHandler.get(name);
}

std::shared_ptr<scene::GameCamera> ResourceManager::createGameCamera(
    const scene::GameCameraCreateInfo& creatInfo) {
    return gameCameraHandler.load(creatInfo);
}

std::shared_ptr<scene::GameCamera> ResourceManager::getGameCamera(
    const std::string& name) {
    return gameCameraHandler.get(name);
}
}  // namespace game
