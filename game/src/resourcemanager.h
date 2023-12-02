#pragma once

#include "sponge.h"
#include <absl/container/flat_hash_map.h>
#include <memory>
#include <string>

class ResourceManager {
   public:
    static std::shared_ptr<sponge::graphics::OrthoCamera> createOrthoCamera(
        const std::string& name);
    static std::shared_ptr<sponge::graphics::OrthoCamera> getOrthoCamera(
        const std::string& name);

   private:
    ResourceManager() = default;

    static absl::flat_hash_map<std::string,
                               std::shared_ptr<sponge::graphics::OrthoCamera>>
        cameras;
};
