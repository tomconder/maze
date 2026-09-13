#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace sponge::core {

class File {
public:
    static std::string getLogDir(const std::string& app);
    static std::string getResourceDir();

    // Whole file as bytes, or at most maxBytes of it. Returns empty on any
    // failure and logs nothing: callers know what the file was for.
    static std::vector<uint8_t> readBytes(
        const std::string& path,
        std::size_t        maxBytes = std::numeric_limits<std::size_t>::max());
};

}  // namespace sponge::core

#if __APPLE__
#include "platform/osx/core/osxfile.hpp"
#elif _WIN32 || WIN32
#include "platform/windows/core/winfile.hpp"
#elif __linux__
#include "platform/linux/core/linuxfile.hpp"
#endif
