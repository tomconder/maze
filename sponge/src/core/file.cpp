#include "core/file.hpp"

namespace sponge::core {

std::string File::getLogDir(const std::string& app) {
    std::string result = {};

#ifdef __APPLE__
    result = OSXFile::getLogDir(app);
#elif defined(_WIN32) || defined(WIN32)
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

}  // namespace sponge::core
