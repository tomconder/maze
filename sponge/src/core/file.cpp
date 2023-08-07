#include "core/file.h"

namespace sponge {

std::string File::getLogDir() {
    std::string result = {};

#ifdef __APPLE__
    result = OSXFile::getOSXLogDir();
#endif

    return result;
}

std::string File::getResourceDir() {
    std::string result = "assets";

#ifdef __APPLE__
    result = OSXFile::getOSXResourceDir();
#endif

    return result;
}

}  // namespace sponge
