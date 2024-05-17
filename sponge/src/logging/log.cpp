#include "log.hpp"
#include "logflag.hpp"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace sponge::logging {

std::shared_ptr<spdlog::logger> Log::appLogger;
std::shared_ptr<spdlog::logger> Log::coreLogger;
std::shared_ptr<spdlog::logger> Log::glLogger;

std::string_view Log::colorFormatPattern = "%^%*%m%d %T.%f %7t %s:%# [%n] %v%$";
std::string_view Log::fileFormatPattern = "%*%m%d %T.%f %7t %s:%# [%n] %v";
std::string_view Log::guiFormatPattern = "%*%m%d %T.%f %7t %s:%# [%n] %v";

void Log::init(const std::string_view logfile) {
    const auto console = spdlog::stdout_color_mt("console");
    set_default_logger(console);

    spdlog::set_level(spdlog::level::trace);

    std::vector<spdlog::sink_ptr> sinks;

    const auto colorSink =
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    setFormatter(colorSink, colorFormatPattern.data());
    sinks.emplace_back(colorSink);

    const auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        logfile.data(), true);
    setFormatter(fileSink, fileFormatPattern.data());
    sinks.emplace_back(fileSink);

    coreLogger = registerLogger("SPONGE", sinks);
    appLogger = registerLogger("APP", sinks);
    glLogger = registerLogger("OPENGL", sinks);
}

void Log::addSink(const spdlog::sink_ptr ptr, const std::string_view pattern) {
    setFormatter(ptr, pattern.data());

    coreLogger->sinks().push_back(ptr);
    appLogger->sinks().push_back(ptr);
    glLogger->sinks().push_back(ptr);
}

void Log::setFormatter(const spdlog::sink_ptr ptr,
                       const std::string_view pattern) {
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<LogFlag>('*').set_pattern(pattern.data());
    ptr->set_formatter(std::move(formatter));
}

std::shared_ptr<spdlog::logger> Log::registerLogger(
    const std::string_view name, const std::vector<spdlog::sink_ptr> sinks) {
    auto logger = std::make_shared<spdlog::logger>(name.data(), sinks.begin(),
                                                   sinks.end());
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::trace);
    register_logger(logger);

    return logger;
}

}  // namespace sponge::logging
