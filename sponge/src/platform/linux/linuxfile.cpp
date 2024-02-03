#include "linuxfile.h"
#include <cstrlib>
#include <filesystem>

namespace sponge {

std::string LinuxFile::getLogDir(const std::string& app) {
    char* appdata = nullptr;
    size_t sz = 0;
    auto val = std::getenv("XDG_DATA_HOME");
    if (val != nullptr) {
        std::filesystem::path path(appdata);
        return path.string() + "/" + app + "/";
    }

    val = std::getenv("HOME");
    if (val != nullptr) {
        std::filesystem::path path(appdata);
        return path.string() + "/.local/share/" + app + "/";
    }

    throw std::runtime_error("Failed to get pref folder");
}

}  // namespace sponge
