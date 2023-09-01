#pragma once

#include "sponge.h"
#include <memory>
#include <string>
#include <unordered_map>

class ResourceManager {
   public:
    static std::shared_ptr<sponge::OrthoCamera> createOrthoCamera(
        const std::string& name);
    static std::shared_ptr<sponge::OrthoCamera> getOrthoCamera(
        const std::string& name);

   private:
    ResourceManager() = default;

    static std::unordered_map<std::string, std::shared_ptr<sponge::OrthoCamera>>
        cameras;
};
