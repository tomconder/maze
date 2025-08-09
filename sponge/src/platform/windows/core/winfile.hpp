#pragma once

#include <string>

namespace sponge::platform::windows::core {

class WinFile {
public:
    static std::string getLogDir(const std::string& app);
};

}  // namespace sponge::platform::windows::core
