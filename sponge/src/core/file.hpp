#pragma once

#include <string>

namespace sponge::core {

class File {
public:
    static std::string getLogDir(const std::string& app);
    static std::string getResourceDir();
};

}  // namespace sponge::core

#if __APPLE__
#include "platform/osx/core/osxfile.hpp"
#elif _WIN32 || WIN32
#include "platform/windows/core/winfile.hpp"
#elif __linux__
#include "platform/linux/core/linuxfile.hpp"
#endif
