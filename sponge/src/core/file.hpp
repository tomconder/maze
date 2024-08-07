#pragma once

#include <string>

namespace sponge {

class File {
   public:
    static std::string getLogDir(const std::string& app);
    static std::string getResourceDir();
};

}  // namespace sponge

#ifdef __APPLE__
#include "platform/osx/osxfile.hpp"
#elif defined(_WIN32) || defined(WIN32)
#include "platform/windows/winfile.hpp"
#elif __UNIX__
#include "platform/linux/linuxfile.hpp"
#endif
