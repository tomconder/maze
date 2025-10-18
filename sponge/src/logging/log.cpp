#include "logging/log.hpp"

#include "logging/logflag.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sponge::logging {
std::shared_ptr<spdlog::logger> Log::appLogger;
std::shared_ptr<spdlog::logger> Log::coreLogger;
std::shared_ptr<spdlog::logger> Log::glLogger;

void Log::init(const std::string& logfile) {
    const auto console = spdlog::stdout_color_mt("console");
    set_default_logger(console);

    spdlog::set_level(spdlog::level::trace);

    std::vector<spdlog::sink_ptr> sinks;

    const auto colorSink =
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    setFormatter(colorSink, colorFormatPattern);
    sinks.emplace_back(colorSink);

    const auto fileSink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(logfile, true);
    setFormatter(fileSink, fileFormatPattern);
    sinks.emplace_back(fileSink);

    coreLogger = registerLogger("SPONGE", sinks);
    appLogger = registerLogger("APP", sinks);
    glLogger = registerLogger("OPENGL", sinks);
}

void Log::shutdown() {
    appLogger.reset();
    coreLogger.reset();
    glLogger.reset();
    spdlog::shutdown();
}

void Log::addSink(const spdlog::sink_ptr& sink, const std::string& pattern) {
    setFormatter(sink, pattern);

    coreLogger->sinks().emplace_back(sink);
    appLogger->sinks().emplace_back(sink);
    glLogger->sinks().emplace_back(sink);
}

void Log::setFormatter(const spdlog::sink_ptr& sink,
                       const std::string& pattern) {
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<LogFlag>('*').set_pattern(pattern);
    sink->set_formatter(std::move(formatter));
}

std::shared_ptr<spdlog::logger> Log::registerLogger(
    const std::string& name, const std::vector<spdlog::sink_ptr>& sinks) {
    auto logger =
        std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::trace);
    register_logger(logger);

    return logger;
}

}  // namespace sponge::logging
