#include "base.hpp"
#include "file.hpp"
#include "log.hpp"

constexpr std::string_view spongeLogFile = "log.txt";
constexpr std::string_view appFolder = "ItsTom";

#define ASNFAM1

namespace sponge {
void startupCore() {
    const auto logfile =
        File::getLogDir(appFolder.data()) + spongeLogFile.data();
    Log::init(logfile);
}

void shutdownCore() {
    Log::shutdown();
}

}  // namespace sponge
