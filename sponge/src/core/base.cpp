#include "core/base.hpp"

#include "core/file.hpp"
#include "core/settings.hpp"
#include "logging/log.hpp"

#include <filesystem>
#include <string>

namespace {
inline const std::string spongeLogFile      = "log.txt";
inline const std::string spongeSettingsFile = "settings.json";
inline const std::string appFolder          = "ItsTom";
}  // namespace

namespace sponge::core {

using logging::Log;

void startupCore() {
    const auto path    = std::filesystem::path(File::getLogDir(appFolder));
    const auto logfile = (path / spongeLogFile).string();
    Log::init(logfile);

    const auto settingsFile = (path / spongeSettingsFile).string();
    Settings::load(settingsFile);
}

void shutdownCore() {
    Log::shutdown();
}

}  // namespace sponge::core
