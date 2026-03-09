#include "platform/windows/core/winfile.hpp"

#include <filesystem>

namespace sponge::platform::windows::core {

std::string WinFile::getLogDir(const std::string& app) {
    char*  appdata = nullptr;
    size_t sz      = 0;
    if (_dupenv_s(&appdata, &sz, "LOCALAPPDATA") == 0 && appdata != nullptr) {
        std::string result = (std::filesystem::path(appdata) / app).string();
        free(appdata);
        return result;
    }
    free(appdata);

    throw std::runtime_error("Failed to get appdata folder");
}

}  // namespace sponge::platform::windows::core
