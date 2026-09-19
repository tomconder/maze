#include "platform/audio/audio.hpp"

#include "core/settings.hpp"
#include "logging/log.hpp"

#include "miniaudio.h"

#include <string>

namespace {
ma_engine      engine{};
ma_sound_group sfxGroup{};
ma_sound_group musicGroup{};
bool           initialized = false;

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
    initialized = false;
}

void Audio::play(const std::string_view path) {
    if (!initialized || path.empty()) {
        return;
    }

    const std::string file(path);
    const auto result = ma_engine_play_sound(&engine, file.c_str(), &sfxGroup);
    if (result != MA_SUCCESS) {
        SPONGE_CORE_WARN("Unable to play sound {}: {}", file,
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
