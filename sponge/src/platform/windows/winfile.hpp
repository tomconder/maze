#pragma once

#include <string>

namespace sponge {

class WinFile {
   public:
    static std::string getLogDir(const std::string& app);
};

}  // namespace sponge
