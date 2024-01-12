#include "core/log.h"
#include "core/logflag.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace sponge {

std::shared_ptr<spdlog::logger> Log::appLogger;
std::shared_ptr<spdlog::logger> Log::coreLogger;
std::shared_ptr<spdlog::logger> Log::glLogger;

void Log::init(std::string_view logfile) {
    std::vector<spdlog::sink_ptr> logSinks;
    logSinks.emplace_back(
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        logfile.data(), true));

    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);

    auto colorFormatter = std::make_unique<spdlog::pattern_formatter>();
    colorFormatter->add_flag<LogFlag>('*').set_pattern(
        "%^%*%m%d %T.%f %t [%s:%#] %n: %v%$");
    logSinks[0]->set_formatter(std::move(colorFormatter));

    auto fileFormatter = std::make_unique<spdlog::pattern_formatter>();
    fileFormatter->add_flag<LogFlag>('*').set_pattern(
        "%*%m%d %T.%f %t [%s:%#] %n: %v");
    logSinks[1]->set_formatter(std::move(fileFormatter));

    coreLogger = std::make_shared<spdlog::logger>("SPONGE", begin(logSinks),
                                                  end(logSinks));
    spdlog::register_logger(coreLogger);
    coreLogger->set_level(spdlog::level::trace);
    coreLogger->flush_on(spdlog::level::trace);

    appLogger =
        std::make_shared<spdlog::logger>("APP", begin(logSinks), end(logSinks));
    spdlog::register_logger(appLogger);
    appLogger->set_level(spdlog::level::trace);
    appLogger->flush_on(spdlog::level::trace);

    glLogger = std::make_shared<spdlog::logger>("OPENGL", begin(logSinks),
                                                end(logSinks));
    spdlog::register_logger(glLogger);
    glLogger->set_level(spdlog::level::trace);
    glLogger->flush_on(spdlog::level::trace);
}

}  // namespace sponge
