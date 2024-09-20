#include "linuxfile.hpp"
#include <cstdlib>
#include <filesystem>

namespace sponge::platform::linux::core {

std::string LinuxFile::getLogDir(const std::string& app) {
    auto val = std::getenv("XDG_DATA_HOME");
    if (val != nullptr) {
        std::filesystem::path path(val);
        return path.string() + "/" + app + "/";
    }

    val = std::getenv("HOME");
    if (val != nullptr) {
        std::filesystem::path path(val);
        return path.string() + "/.local/share/" + app + "/";
    }

    throw std::runtime_error("Failed to get pref folder");
}

}  // namespace sponge
