#include "logging/log.hpp"

#include <fmt/format.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <iterator>
#include <memory>
#include <string>
#include <vector>

namespace sponge::logging {
// Log::write and the log sinks cast between the two level types
static_assert(static_cast<int>(Level::Trace) ==
              static_cast<int>(spdlog::level::trace));
static_assert(static_cast<int>(Level::Debug) ==
              static_cast<int>(spdlog::level::debug));
static_assert(static_cast<int>(Level::Info) ==
              static_cast<int>(spdlog::level::info));
static_assert(static_cast<int>(Level::Warn) ==
              static_cast<int>(spdlog::level::warn));
static_assert(static_cast<int>(Level::Error) ==
              static_cast<int>(spdlog::level::err));
static_assert(static_cast<int>(Level::Critical) ==
              static_cast<int>(spdlog::level::critical));

namespace {
std::shared_ptr<spdlog::logger> appLogger;
std::shared_ptr<spdlog::logger> coreLogger;
std::shared_ptr<spdlog::logger> glLogger;

std::shared_ptr<spdlog::logger>
    registerLogger(const std::string&                   name,
                   const std::vector<spdlog::sink_ptr>& sinks) {
    auto logger =
        std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::trace);
    register_logger(logger);

    return logger;
}
}  // namespace

void Log::init(const std::string& logfile) {
    spdlog::set_level(
        static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));

    std::vector<spdlog::sink_ptr> sinks;

    const auto colorSink =
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    colorSink->set_pattern(colorFormatPattern);
    sinks.emplace_back(colorSink);

    const auto fileSink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(logfile, true);
    fileSink->set_pattern(fileFormatPattern);
    sinks.emplace_back(fileSink);

    coreLogger = registerLogger("SPONGE", sinks);
    appLogger  = registerLogger("APP", sinks);
    glLogger   = registerLogger("OPENGL", sinks);
}

void Log::shutdown() {
    appLogger.reset();
    coreLogger.reset();
    glLogger.reset();
    spdlog::shutdown();
}

void Log::addSink(const spdlog::sink_ptr& sink, const std::string& pattern) {
    sink->set_pattern(pattern);

    coreLogger->sinks().emplace_back(sink);
    appLogger->sinks().emplace_back(sink);
    glLogger->sinks().emplace_back(sink);
}

void Log::write(const Channel channel, const Level level, const char* file,
                const int line, const char* function,
                const fmt::string_view format, const fmt::format_args args) {
    const auto& logger   = channel == Channel::App  ? appLogger :
                           channel == Channel::Core ? coreLogger :
                                                      glLogger;
    const auto  spdLevel = static_cast<spdlog::level::level_enum>(level);
    if (!logger->should_log(spdLevel)) {
        return;
    }

    const spdlog::source_loc location{ file, line, function };
    try {
        fmt::memory_buffer message;
        fmt::vformat_to(std::back_inserter(message), format, args);
        logger->log(location, spdLevel,
                    spdlog::string_view_t(message.data(), message.size()));
    } catch (const fmt::format_error& e) {
        logger->log(location, spdlog::level::err, e.what());
    }
}

}  // namespace sponge::logging
