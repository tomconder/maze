#include "logging/log.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <memory>
#include <string>
#include <vector>

namespace sponge::logging {
std::shared_ptr<spdlog::logger> Log::appLogger;
std::shared_ptr<spdlog::logger> Log::coreLogger;
std::shared_ptr<spdlog::logger> Log::glLogger;

const char Log::colorFormatPattern[] = "%^%L%m%d %T.%f %7t %s:%# [%n] %v%$";
const char Log::fileFormatPattern[]  = "%L%m%d %T.%f %7t %s:%# [%n] %v";

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

std::shared_ptr<spdlog::logger>
    Log::registerLogger(const std::string&                   name,
                        const std::vector<spdlog::sink_ptr>& sinks) {
    auto logger =
        std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::trace);
    register_logger(logger);

    return logger;
}

}  // namespace sponge::logging
