#pragma once

#include <string>

namespace sponge {

class LinuxFile {
   public:
    static std::string getLogDir(const std::string& app);
};

}  // namespace sponge
