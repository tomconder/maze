#include "core/file.h"

namespace sponge {

std::string File::getLogDir(const std::string& app) {
    std::string result = {};

#ifdef __APPLE__
    result = OSXFile::getLogDir(app);
#elif __WINDOWS__
    result = WinFile::getLogDir(app);
#elif __UNIX__
    result = LinuxFile::getLogDir(app);
#endif

    return result;
}

std::string File::getResourceDir() {
    std::string result = "assets";

#ifdef __APPLE__
    result = OSXFile::getResourceDir();
#endif

    return result;
}

}  // namespace sponge
