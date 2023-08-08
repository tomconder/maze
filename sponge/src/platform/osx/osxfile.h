#pragma once

#include <string>

namespace sponge {

class OSXFile {
   public:
    static std::string getLogDir();
    static std::string getResourceDir();

   private:
    static std::string expandTilde(const char* str);
};

}  // namespace sponge
