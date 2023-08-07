#pragma once

#include "core/base.h"
#include <string>

namespace sponge {

class File {
   public:
    static std::string getLogDir();
    static std::string getResourceDir();
};

}  // namespace sponge

#ifdef __APPLE__
#include "platform/osx/osxfile.h"
#endif
