#include "platform/osx/osxfile.h"
#include <CoreFoundation/CFBundle.h>
#include <glob.h>
#include <sysdir.h>

namespace sponge {

std::string OSXFile::expandTilde(const char* str) {
    glob_t globbuf;
    if (glob(str, GLOB_TILDE, nullptr, &globbuf) == 0) {
        std::string result(globbuf.gl_pathv[0]);
        globfree(&globbuf);
        return result;
    }

    throw std::runtime_error("Unable to expand tilde");
}

std::string OSXFile::getResourceDir() {
    CFURLRef resourceURL =
        CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
    char resourcePath[PATH_MAX];

    if (CFURLGetFileSystemRepresentation(
            resourceURL, 1, reinterpret_cast<UInt8*>(resourcePath), PATH_MAX)) {
        if (resourceURL != nullptr) {
            CFRelease(resourceURL);
        }
        return resourcePath;
    }

    return "../Resources";
}

std::string OSXFile::getLogDir(const std::string& app) {
    char path[PATH_MAX];

    auto state = sysdir_start_search_path_enumeration(
        SYSDIR_DIRECTORY_APPLICATION_SUPPORT, SYSDIR_DOMAIN_MASK_USER);
    if (sysdir_get_next_search_path_enumeration(state, path) != 0) {
        return expandTilde(path) + "/" + app + "/";
    }

    throw std::runtime_error("Failed to get settings folder");
}

}  // namespace sponge
