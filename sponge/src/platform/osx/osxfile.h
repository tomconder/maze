#pragma once

#include <string>

namespace sponge {

class OSXFile {
   public:
    static std::string getOSXLogDir();
    static std::string getOSXResourceDir();

   private:
    static std::string expandTilde(const char* str);
};

}  // namespace sponge
