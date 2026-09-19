#include "scene/scenefile.hpp"

#include "core/file.hpp"
#include "logging/log.hpp"

#include <fkYAML/node.hpp>
#include <glm/trigonometric.hpp>

#include <fstream>
#include <string>
#include <utility>

namespace {
using fkyaml::node;

// Every reader keeps the fallback unless the key exists and holds a type it
// can convert, so a typo degrades to the default instead of to an exception.
const node* find(const node& map, const char* key) {
    if (!map.is_mapping() || !map.contains(key)) {
        return nullptr;
    }
    return &map[key];
}

float toFloat(const node& value, const float fallback) {
    // YAML tells 10 and 10.0 apart; fkYAML converts either to float, so a
    // scene author does not have to. The guard is what keeps a wrong type a
    // fallback instead of an exception that abandons the rest of the parse.
    return value.is_float_number() || value.is_integer() ?
               value.get_value<float>() :
               fallback;
}

float readFloat(const node& map, const char* key, const float fallback) {
    const auto* value = find(map, key);
    return value == nullptr ? fallback : toFloat(*value, fallback);
}

template <typename T>
T readInt(const node& map, const char* key, const T fallback) {
    const auto* value = find(map, key);
    return value == nullptr || !value->is_integer() ? fallback :
                                                      value->get_value<T>();
}

bool readBool(const node& map, const char* key, const bool fallback) {
    const auto* value = find(map, key);
    return value == nullptr || !value->is_boolean() ? fallback :
                                                      value->get_value<bool>();
}

std::string readString(const node& map, const char* key) {
    const auto* value = find(map, key);
    return value == nullptr || !value->is_string() ?
               std::string{} :
               value->get_value<std::string>();
}

glm::vec3 readVec3(const node& map, const char* key, const glm::vec3 fallback) {
    const auto* value = find(map, key);
    if (value == nullptr || !value->is_sequence() || value->size() != 3) {
        return fallback;
    }
    return { toFloat((*value)[0], fallback.x), toFloat((*value)[1], fallback.y),
             toFloat((*value)[2], fallback.z) };
}

void readCamera(const node& root, game::scene::SceneCamera& camera) {
    const auto* map = find(root, "camera");
    if (map == nullptr) {
        return;
    }
    camera.position = readVec3(*map, "position", camera.position);
    camera.fov      = readFloat(*map, "fov", camera.fov);
    camera.yaw      = readFloat(*map, "yaw", camera.yaw);
    camera.pitch    = readFloat(*map, "pitch", camera.pitch);
}

void readLighting(const node& root, game::scene::SceneLighting& lighting) {
    const auto* map = find(root, "lighting");
    if (map == nullptr) {
        return;
    }

    if (const auto* ambient = find(*map, "ambient"); ambient != nullptr) {
        auto& dst     = lighting.ambient;
        dst.strength  = readFloat(*ambient, "strength", dst.strength);
        dst.occlusion = readFloat(*ambient, "occlusion", dst.occlusion);
    }

    if (const auto* directional = find(*map, "directional");
        directional != nullptr) {
        auto& dst      = lighting.directional;
        dst.enabled    = readBool(*directional, "enabled", dst.enabled);
        dst.castShadow = readBool(*directional, "castShadow", dst.castShadow);
        dst.color      = readVec3(*directional, "color", dst.color);
        dst.direction  = readVec3(*directional, "direction", dst.direction);
        dst.shadowMapRes =
            readInt(*directional, "shadowMapRes", dst.shadowMapRes);
    }

    if (const auto* point = find(*map, "point"); point != nullptr) {
        auto& dst        = lighting.point;
        dst.count        = readInt(*point, "count", dst.count);
        dst.radiusMin    = readFloat(*point, "radiusMin", dst.radiusMin);
        dst.radiusSpan   = readFloat(*point, "radiusSpan", dst.radiusSpan);
        dst.height       = readFloat(*point, "height", dst.height);
        dst.jitterAngle  = readFloat(*point, "jitterAngle", dst.jitterAngle);
        dst.jitterRadius = readFloat(*point, "jitterRadius", dst.jitterRadius);
        dst.color        = readVec3(*point, "color", dst.color);
        dst.attenuationIndex =
            readInt(*point, "attenuationIndex", dst.attenuationIndex);
        dst.debugCubeScale =
            readFloat(*point, "debugCubeScale", dst.debugCubeScale);
    }
}

void readBloom(const node& root, game::scene::SceneBloom& bloom) {
    const auto* map = find(root, "bloom");
    if (map == nullptr) {
        return;
    }
    bloom.threshold = readFloat(*map, "threshold", bloom.threshold);
    bloom.intensity = readFloat(*map, "intensity", bloom.intensity);
}

void readObjects(const node&                            root,
                 std::vector<game::scene::SceneObject>& objects) {
    const auto* sequence = find(root, "objects");
    if (sequence == nullptr || !sequence->is_sequence()) {
        return;
    }

    for (const auto& entry : *sequence) {
        game::scene::SceneObject object;
        object.name = readString(entry, "name");
        object.path = readString(entry, "path");

        // Paths used to be compile-time constants. Now a typo is possible, so
        // an unusable entry is dropped here rather than left to desynchronise
        // the model, matrix and emissive arrays the renderer indexes together.
        if (object.name.empty() || object.path.empty()) {
            SPONGE_ERROR("Skipping scene object without a name or path");
            continue;
        }

        object.scale       = readVec3(entry, "scale", object.scale);
        object.translation = readVec3(entry, "translation", object.translation);
        object.emissive    = readVec3(entry, "emissive", object.emissive);

        if (const auto* rotation = find(entry, "rotation");
            rotation != nullptr) {
            object.rotation.angle =
                glm::radians(readFloat(*rotation, "angle", 0.F));
            object.rotation.axis =
                readVec3(*rotation, "axis", object.rotation.axis);
        }

        objects.push_back(std::move(object));
    }
}
}  // namespace

namespace game::scene {

Scene loadScene(const std::string& path) {
    Scene scene;

    const auto    filename = sponge::core::File::getResourceDir() + path;
    std::ifstream file(filename);
    if (!file.is_open()) {
        SPONGE_ERROR("Unable to open scene file: {}", filename);
        return scene;
    }

    SPONGE_INFO("Loading scene file: {}", filename);

    try {
        const auto root = node::deserialize(file);
        readCamera(root, scene.camera);
        readLighting(root, scene.lighting);
        readBloom(root, scene.bloom);
        readObjects(root, scene.objects);
    } catch (const std::exception& e) {
        // Keep whatever parsed before the fault plus the defaults: a bad file
        // must not take the game down on the way up.
        SPONGE_ERROR("Unable to parse scene file {}: {}", filename, e.what());
    }

    return scene;
}

}  // namespace game::scene
