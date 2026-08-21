#pragma once

#include <string_view>

namespace sponge::platform::audio {

class Audio {
public:
    static void init();
    static void shutdown();
    static void play(std::string_view path);
};

}  // namespace sponge::platform::audio
