#include "winfile.h"
#include <filesystem>

namespace sponge {

std::string WinFile::getLogDir() {
    char* appdata = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&appdata, &sz, "LOCALAPPDATA") == 0 && appdata != nullptr) {
        std::filesystem::path path(appdata);
        return path.string() + "/maze/";
    }

    throw std::runtime_error("Failed to get appdata folder");
}

}  // namespace sponge
