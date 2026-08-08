#include "platform/windows/core/winfile.hpp"

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

namespace sponge::platform::windows::core {

std::string WinFile::getLogDir(const std::string& app) {
    char*  rawAppdata = nullptr;
    size_t sz         = 0;
    if (_dupenv_s(&rawAppdata, &sz, "LOCALAPPDATA") != 0 ||
        rawAppdata == nullptr) {
        throw std::runtime_error("Failed to get appdata folder");
    }
    const std::unique_ptr<char, decltype(&free)> appdata(rawAppdata, free);

    return (std::filesystem::path(appdata.get()) / app).string();
}

}  // namespace sponge::platform::windows::core
