#include "base.hpp"
#include "file.hpp"
#include "logging/log.hpp"
#include <filesystem>

namespace {
constexpr char spongeLogFile[] = "log.txt";
constexpr char appFolder[] = "ItsTom";
}  // namespace

namespace sponge::core {

using logging::Log;

void startupCore() {
    const auto path = std::filesystem::path(File::getLogDir(appFolder));
    const auto logfile = std::filesystem::path(File::getLogDir(path / spongeLogFile)).string();
    Log::init(logfile);
}

void shutdownCore() {
    Log::shutdown();
}

}  // namespace sponge::core
