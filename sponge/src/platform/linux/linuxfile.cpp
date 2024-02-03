#include "linuxfile.h"
#include <filesystem>

namespace sponge {

std::string LinuxFile::getLogDir(const std::string& app) {
    char* appdata = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&appdata, &sz, "XDG_DATA_HOME") == 0 && appdata != nullptr) {
        std::filesystem::path path(appdata);
        return path.string() + "/" + app + "/";
    }

    if (_dupenv_s(&appdata, &sz, "HOME") == 0 && appdata != nullptr) {
        std::filesystem::path path(appdata);
        return path.string() + "/.local/share/" + app + "/";
    }

    throw std::runtime_error("Failed to get pref folder");
}

}  // namespace sponge
