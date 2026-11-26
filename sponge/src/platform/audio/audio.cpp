#include "platform/audio/audio.hpp"

#include "core/assetfiles.hpp"
#include "core/settings.hpp"
#include "logging/log.hpp"

#include "miniaudio.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
ma_engine      engine{};
ma_sound_group sfxGroup{};
ma_sound_group musicGroup{};
bool           initialized = false;

// The files of the sounds that have played. miniaudio reads these bytes and
// does not copy them, so they stay until the engine is gone. Each file is held
// whole, which suits short sounds. A long track needs a miniaudio file system
// that reads ranges.
std::unordered_map<std::string, std::vector<uint8_t>> sounds;
std::mutex                                            soundsMutex;

// Gives the sound to miniaudio at its first play. miniaudio then finds it by
// name.
bool loadSound(const std::string& name) {
    const std::lock_guard lock(soundsMutex);
    if (sounds.contains(name)) {
        return true;
    }

    std::string error;
    auto        bytes = sponge::core::assets::read(name, error);
    if (!error.empty()) {
        SPONGE_CORE_WARN("Unable to load sound: {}", error);
        return false;
    }

    const auto& data   = sounds.emplace(name, std::move(bytes)).first->second;
    const auto  result = ma_resource_manager_register_encoded_data(
        ma_engine_get_resource_manager(&engine), name.c_str(), data.data(),
        data.size());
    if (result != MA_SUCCESS) {
        SPONGE_CORE_WARN("Unable to register sound {}: {}", name,
                         static_cast<int>(result));
        sounds.erase(name);
        return false;
    }
    return true;
}

// 0..100 in Settings, 0..1 for miniaudio.
float loadVolumeSetting(const std::string& key) {
    return static_cast<float>(sponge::core::Settings::getUInt32(key, 100)) /
           100.F;
}
}  // namespace

namespace sponge::platform::audio {

void Audio::init() {
    if (initialized) {
        return;
    }

    SPONGE_CORE_INFO("Initializing audio");
    const auto result = ma_engine_init(nullptr, &engine);
    if (result != MA_SUCCESS) {
        SPONGE_CORE_ERROR("Unable to initialize audio engine: {}",
                          static_cast<int>(result));
        return;
    }

    if (const auto sfxResult =
            ma_sound_group_init(&engine, 0, nullptr, &sfxGroup);
        sfxResult != MA_SUCCESS) {
        SPONGE_CORE_ERROR("Unable to initialize sfx sound group: {}",
                          static_cast<int>(sfxResult));
    }
    if (const auto musicResult =
            ma_sound_group_init(&engine, 0, nullptr, &musicGroup);
        musicResult != MA_SUCCESS) {
        SPONGE_CORE_ERROR("Unable to initialize music sound group: {}",
                          static_cast<int>(musicResult));
    }

    initialized = true;

    setMasterVolume(loadVolumeSetting("audio.masterVolume"));
    setSfxVolume(loadVolumeSetting("audio.sfxVolume"));
    setMusicVolume(loadVolumeSetting("audio.musicVolume"));
}

void Audio::shutdown() {
    if (!initialized) {
        return;
    }

    ma_sound_group_uninit(&musicGroup);
    ma_sound_group_uninit(&sfxGroup);
    ma_engine_uninit(&engine);
    // After the engine, which reads these bytes.
    sounds.clear();
    initialized = false;
}

void Audio::play(const std::string_view name) {
    if (!initialized || name.empty()) {
        return;
    }

    const std::string sound(name);
    if (!loadSound(sound)) {
        return;
    }
    const auto result = ma_engine_play_sound(&engine, sound.c_str(), &sfxGroup);
    if (result != MA_SUCCESS) {
        SPONGE_CORE_WARN("Unable to play sound {}: {}", sound,
                         static_cast<int>(result));
    }
}

void Audio::setMasterVolume(const float volume) {
    if (!initialized) {
        return;
    }
    ma_engine_set_volume(&engine, volume);
}

void Audio::setSfxVolume(const float volume) {
    if (!initialized) {
        return;
    }
    ma_sound_group_set_volume(&sfxGroup, volume);
}

void Audio::setMusicVolume(const float volume) {
    if (!initialized) {
        return;
    }
    ma_sound_group_set_volume(&musicGroup, volume);
}

}  // namespace sponge::platform::audio
