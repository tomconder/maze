#include "core/file.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

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

std::vector<uint8_t> File::readBytes(const std::string& path,
                                     const std::size_t  maxBytes) {
    std::ifstream file{ path, std::ios::binary | std::ios::ate };
    if (!file) {
        return {};
    }

    const auto size =
        std::min(static_cast<std::size_t>(file.tellg()), maxBytes);
    file.seekg(0);
    std::vector<uint8_t> bytes(size);
    if (!file.read(reinterpret_cast<char*>(bytes.data()),
                   static_cast<std::streamsize>(size))) {
        return {};
    }
    return bytes;
}

}  // namespace sponge::core
