#include "core/base.hpp"

#include "core/file.hpp"
#include "logging/log.hpp"

#include <filesystem>
#include <string>

namespace {
constexpr char spongeLogFile[] = "log.txt";
constexpr char appFolder[]     = "ItsTom";
}  // namespace

namespace sponge::core {

using logging::Log;

void startupCore() {
    const auto path    = std::filesystem::path(File::getLogDir(appFolder));
    const auto logfile = (path / spongeLogFile).string();
    Log::init(logfile);
}

void shutdownCore() {
    Log::shutdown();
}

}  // namespace sponge::core
