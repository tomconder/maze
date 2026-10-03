#include "scene/orthocamera.hpp"

#include "core/base.hpp"
#include "logging/log.hpp"

#include <glm/ext/matrix_clip_space.hpp>

#include <cstdint>
#include <memory>

namespace game::scene {
OrthoCamera::OrthoCamera(const OrthoCameraCreateInfo& createInfo) {
    SPONGE_INFO("Creating ortho camera: {}", createInfo.name);
    UNUSED(createInfo);
}

void OrthoCamera::setWidthAndHeight(const uint32_t width,
                                    const uint32_t height) {
    w = width;
    h = height;
    updateProjection(w, h);
}

void OrthoCamera::updateProjection(const uint32_t width,
                                   const uint32_t height) {
    projection = glm::ortho(0.F, static_cast<float>(width),
                            static_cast<float>(height), 0.F, -1.F, 1.F);
}

std::shared_ptr<OrthoCamera> menuOrthoCamera() {
    static const auto camera =
        std::make_shared<OrthoCamera>(OrthoCameraCreateInfo{ .name = "menu" });
    return camera;
}

}  // namespace game::scene
