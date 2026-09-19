#pragma once

#include <string_view>

namespace sponge::platform::audio {

class Audio {
public:
    static void init();
    static void shutdown();
    static void play(std::string_view path);

    // 0 (silent) to 1 (full). Master scales both buses; Sfx/Music each scale
    // their own group on top of it.
    static void setMasterVolume(float volume);
    static void setSfxVolume(float volume);
    static void setMusicVolume(float volume);
};

}  // namespace sponge::platform::audio
