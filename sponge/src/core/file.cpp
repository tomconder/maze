#include "core/file.hpp"

#include <string>

namespace sponge::core {

std::string File::getLogDir(const std::string& app) {
    std::string result = {};

#if __APPLE__
    result = platform::osx::core::OSXFile::getLogDir(app);
#elif _WIN32 || WIN32
    result = platform::windows::core::WinFile::getLogDir(app);
#elif __linux__
    result = platform::linux::core::LinuxFile::getLogDir(app);
#endif

    return result;
}

std::string File::getResourceDir() {
    std::string result = "assets";

#if __APPLE__
    result = platform::osx::core::OSXFile::getResourceDir();
#endif

    return result;
}

}  // namespace sponge::core
