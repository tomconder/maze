#include "log.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "logflag.h"

std::shared_ptr<spdlog::logger> Log::coreLogger;
std::shared_ptr<spdlog::logger> Log::appLogger;

void Log::init() {
    std::vector<spdlog::sink_ptr> logSinks;
    logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(SPONGE_LOG_FILE, true));

    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);

    auto colorFormatter = std::make_unique<spdlog::pattern_formatter>();
    colorFormatter->add_flag<LogFlag>('*').set_pattern("%^%*%$ %T,%e [%n] %s:%# - %v");
    logSinks[0]->set_formatter(std::move(colorFormatter));

    auto fileFormatter = std::make_unique<spdlog::pattern_formatter>();
    fileFormatter->add_flag<LogFlag>('*').set_pattern("%* %Y-%m-%d %T,%e [%n] - %v");
    logSinks[1]->set_formatter(std::move(fileFormatter));

    coreLogger = std::make_shared<spdlog::logger>("SPONGE", begin(logSinks), end(logSinks));
    spdlog::register_logger(coreLogger);
    coreLogger->set_level(spdlog::level::trace);
    coreLogger->flush_on(spdlog::level::trace);

    appLogger = std::make_shared<spdlog::logger>("APP", begin(logSinks), end(logSinks));
    spdlog::register_logger(appLogger);
    appLogger->set_level(spdlog::level::trace);
    appLogger->flush_on(spdlog::level::trace);
}
