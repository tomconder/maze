#include "platform/audio/audio.hpp"

#include "logging/log.hpp"

#include "miniaudio.h"

#include <string>

namespace {
ma_engine engine{};
bool      initialized = false;
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

    initialized = true;
}

void Audio::shutdown() {
    if (!initialized) {
        return;
    }

    ma_engine_uninit(&engine);
    initialized = false;
}

void Audio::play(const std::string_view path) {
    if (!initialized || path.empty()) {
        return;
    }

    const std::string file(path);
    const auto result = ma_engine_play_sound(&engine, file.c_str(), nullptr);
    if (result != MA_SUCCESS) {
        SPONGE_CORE_WARN("Unable to play sound {}: {}", file,
                         static_cast<int>(result));
    }
}

}  // namespace sponge::platform::audio
