#pragma once

#include <string>

namespace sponge::platform::linux::core {

class LinuxFile {
   public:
    static std::string getLogDir(const std::string& app);
};

}  // namespace sponge
