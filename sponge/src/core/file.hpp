#pragma once

#include <string>

namespace sponge::core {

class File {
public:
    static std::string getLogDir(const std::string& app);
    static std::string getResourceDir();
};

}  // namespace sponge::core

#ifdef __APPLE__
#include "platform/osx/core/osxfile.hpp"
#elif defined(_WIN32) || defined(WIN32)
#include "platform/windows/core/winfile.hpp"
#elif __UNIX__
#include "platform/linux/core/linuxfile.hpp"
#endif
