#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace game::scene {

struct SceneObject {
    std::string name;
    std::string path;
    glm::vec3   scale{ 1.F };
    struct {
        // Radians. The file states degrees; loadScene() converts.
        float     angle{ 0.F };
        glm::vec3 axis{ 0.F, 1.F, 0.F };
    } rotation{};
    glm::vec3 translation{ 0.F };
    glm::vec3 emissive{ 0.F };
};

struct SceneCamera {
    glm::vec3 position{ -16.F, 4.F, 0.F };
    float     fov{ 60.F };
    // Degrees, like the camera itself. Yaw 0 looks down +X; pitch clamps to
    // the poles.
    float yaw{ 0.F };
    float pitch{ 0.F };
};

struct SceneAmbient {
    float strength{ .25F };
    // 1 = no extra attenuation. SSAO now supplies real per-pixel occlusion;
    // this is a flat multiplier on top of it, for an artist to darken ambient
    // further without touching SSAO's radius/bias.
    float occlusion{ 1.F };
};

struct SceneDirectionalLight {
    bool      enabled{ true };
    bool      castShadow{ true };
    glm::vec3 color{ 1.F };
    glm::vec3 direction{ 0.F, -20.F, 1.333F };
    // The default only. A saved video.shadowRes overrides it.
    uint32_t shadowMapRes{ 1024U };
};

// Point lights are placed by a seeded spiral rather than authored one by one,
// so the light count stays adjustable at run time without the file and the
// count disagreeing about positions.
struct ScenePointLights {
    int32_t   count{ 0 };
    float     radiusMin{ 1.5F };
    float     radiusSpan{ 13.5F };
    float     height{ 10.F };
    float     jitterAngle{ .4F };
    float     jitterRadius{ .5F };
    glm::vec3 color{ 1.F };
    int32_t   attenuationIndex{ 4 };
    float     debugCubeScale{ .1F };
};

struct SceneLighting {
    SceneAmbient          ambient;
    SceneDirectionalLight directional;
    ScenePointLights      point;
};

// Both act on LINEAR radiance before tone mapping, so they are tuned to this
// scene's lighting. The debug sliders change them for the current run only.
struct SceneBloom {
    float threshold{ .8F };
    float intensity{ .08F };
};

struct Scene {
    SceneCamera              camera;
    SceneLighting            lighting;
    SceneBloom               bloom;
    std::vector<SceneObject> objects;
};

// Reads a YAML scene description, with path taken relative to the resource
// directory. Every key is optional and falls back to the value above, so a
// partial file still yields a complete scene. A missing or malformed file
// logs and yields the defaults with no objects.
Scene loadScene(const std::string& path);

}  // namespace game::scene
