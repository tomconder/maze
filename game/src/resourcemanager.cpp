#include "resourcemanager.h"

absl::flat_hash_map<std::string,
                    std::shared_ptr<sponge::renderer::OrthoCamera>>
    ResourceManager::cameras;

std::shared_ptr<sponge::renderer::OrthoCamera>
ResourceManager::createOrthoCamera(const std::string& name) {
    assert(!name.empty());

    if (cameras.find(name) != cameras.end()) {
        return cameras[name];
    }

    SPONGE_INFO("Creating camera: {}", name);

    auto camera = std::make_shared<sponge::renderer::OrthoCamera>();
    cameras[name] = camera;

    return camera;
}

std::shared_ptr<sponge::renderer::OrthoCamera>
ResourceManager::getOrthoCamera(const std::string& name) {
    assert(!name.empty());
    return cameras.at(name);
}
