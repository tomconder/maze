#include "winfile.hpp"
#include <filesystem>

namespace sponge {

std::string WinFile::getLogDir(const std::string& app) {
    char* appdata = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&appdata, &sz, "LOCALAPPDATA") == 0 && appdata != nullptr) {
        const std::filesystem::path path(appdata);
        return path.string() + "/" + app + "/";
    }

    throw std::runtime_error("Failed to get appdata folder");
}

}  // namespace sponge
