#include "resourcemanager.h"

absl::flat_hash_map<std::string, std::shared_ptr<sponge::graphics::OrthoCamera>>
    ResourceManager::cameras;

std::shared_ptr<sponge::graphics::OrthoCamera> ResourceManager::createOrthoCamera(
    const std::string& name) {
    assert(!name.empty());

    if (cameras.find(name) != cameras.end()) {
        return cameras[name];
    }

    auto camera = std::make_shared<sponge::graphics::OrthoCamera>();
    cameras[name] = camera;

    return camera;
}

std::shared_ptr<sponge::graphics::OrthoCamera> ResourceManager::getOrthoCamera(
    const std::string& name) {
    assert(!name.empty());
    return cameras.at(name);
}
