#include "resourcemanager.hpp"

namespace game {

std::unordered_map<std::string, std::shared_ptr<sponge::scene::OrthoCamera>>
    ResourceManager::orthoCameras;
std::unordered_map<std::string, std::shared_ptr<scene::GameCamera>>
    ResourceManager::gameCameras;

std::shared_ptr<sponge::scene::OrthoCamera> ResourceManager::createOrthoCamera(
    const std::string& name) {
    assert(!name.empty());

    if (orthoCameras.find(name) != orthoCameras.end()) {
        return orthoCameras[name];
    }

    SPONGE_INFO("Creating ortho camera: {}", name);

    auto camera = std::make_shared<sponge::scene::OrthoCamera>();
    orthoCameras[name] = camera;

    return camera;
}

std::shared_ptr<sponge::scene::OrthoCamera> ResourceManager::getOrthoCamera(
    const std::string& name) {
    assert(!name.empty());
    return orthoCameras.at(name);
}

std::shared_ptr<scene::GameCamera> ResourceManager::createGameCamera(
    const std::string& name) {
    assert(!name.empty());

    if (gameCameras.find(name) != gameCameras.end()) {
        return gameCameras[name];
    }

    SPONGE_INFO("Creating game camera: {}", name);

    auto camera = std::make_shared<scene::GameCamera>();
    gameCameras[name] = camera;

    return camera;
}

std::shared_ptr<scene::GameCamera> ResourceManager::getGameCamera(
    const std::string& name) {
    assert(!name.empty());
    return gameCameras.at(name);
}

}  // namespace game
