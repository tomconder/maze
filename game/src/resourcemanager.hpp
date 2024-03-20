#pragma once

#include "sponge.hpp"
#include <absl/container/flat_hash_map.h>
#include <memory>
#include <string>

class ResourceManager {
   public:
    static std::shared_ptr<sponge::renderer::OrthoCamera> createOrthoCamera(
        const std::string& name);
    static std::shared_ptr<sponge::renderer::OrthoCamera> getOrthoCamera(
        const std::string& name);

   private:
    ResourceManager() = default;

    static absl::flat_hash_map<std::string,
                               std::shared_ptr<sponge::renderer::OrthoCamera>>
        cameras;
};
