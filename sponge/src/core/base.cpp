#include "base.hpp"
#include "file.hpp"
#include "logging/log.hpp"

namespace {
constexpr char spongeLogFile[] = "log.txt";
constexpr char appFolder[] = "ItsTom";
}  // namespace

#define ASNFAM1

namespace sponge::core {

using logging::Log;

void startupCore() {
    const auto logfile = File::getLogDir(appFolder) + spongeLogFile;
    Log::init(logfile);
}

void shutdownCore() {
    Log::shutdown();
}

}  // namespace sponge::core
