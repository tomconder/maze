#pragma once

#ifndef SPDLOG_H
#if !NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_ERROR
#endif
#include "spdlog/spdlog.h"
#endif

// include custom log formatters
// ReSharper disable once CppUnusedIncludeDirective
#include "logging/logcustom.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sponge::logging {
struct LogItem {
    std::string message;
    std::string loggerName;
    spdlog::level::level_enum level;
};

class Log {
public:
    static void init(const std::string& logfile);

    static void shutdown();

    static std::shared_ptr<spdlog::logger> getAppLogger() {
        return appLogger;
    }

    static std::shared_ptr<spdlog::logger> getCoreLogger() {
        return coreLogger;
    }

    static std::shared_ptr<spdlog::logger> getGlLogger() {
        return glLogger;
    }

    static void addSink(const spdlog::sink_ptr& sink,
                        const std::string& pattern);

    static constexpr char colorFormatPattern[] =
        "%^%*%m%d %T.%f %7t %s:%# [%n] %v%$";
    static constexpr char fileFormatPattern[] =
        "%*%m%d %T.%f %7t %s:%# [%n] %v";
    static constexpr char guiFormatPattern[] = "%*%m%d %T.%f %7t %s:%# [%n] %v";

private:
    static std::shared_ptr<spdlog::logger> appLogger;
    static std::shared_ptr<spdlog::logger> coreLogger;
    static std::shared_ptr<spdlog::logger> glLogger;

    static std::shared_ptr<spdlog::logger> registerLogger(
        const std::string& name, const std::vector<spdlog::sink_ptr>& sinks);

    static void setFormatter(const spdlog::sink_ptr& sink,
                             const std::string& pattern);
};

#define SPONGE_CORE_TRACE(...) \
    SPDLOG_LOGGER_TRACE(logging::Log::getCoreLogger(), __VA_ARGS__)
#define SPONGE_CORE_DEBUG(...) \
    SPDLOG_LOGGER_DEBUG(logging::Log::getCoreLogger(), __VA_ARGS__)
#define SPONGE_CORE_INFO(...) \
    SPDLOG_LOGGER_INFO(logging::Log::getCoreLogger(), __VA_ARGS__)
#define SPONGE_CORE_WARN(...) \
    SPDLOG_LOGGER_WARN(logging::Log::getCoreLogger(), __VA_ARGS__)
#define SPONGE_CORE_ERROR(...) \
    SPDLOG_LOGGER_ERROR(logging::Log::getCoreLogger(), __VA_ARGS__)
#define SPONGE_CORE_CRITICAL(...) \
    SPDLOG_LOGGER_CRITICAL(logging::Log::getCoreLogger(), __VA_ARGS__)

#define SPONGE_GL_TRACE(...) \
    SPDLOG_LOGGER_TRACE(sponge::logging::Log::getGlLogger(), __VA_ARGS__)
#define SPONGE_GL_DEBUG(...) \
    SPDLOG_LOGGER_DEBUG(sponge::logging::Log::getGlLogger(), __VA_ARGS__)
#define SPONGE_GL_INFO(...) \
    SPDLOG_LOGGER_INFO(sponge::logging::Log::getGlLogger(), __VA_ARGS__)
#define SPONGE_GL_WARN(...) \
    SPDLOG_LOGGER_WARN(sponge::logging::Log::getGlLogger(), __VA_ARGS__)
#define SPONGE_GL_ERROR(...) \
    SPDLOG_LOGGER_ERROR(sponge::logging::Log::getGlLogger(), __VA_ARGS__)
#define SPONGE_GL_CRITICAL(...) \
    SPDLOG_LOGGER_CRITICAL(sponge::logging::Log::getGlLogger(), __VA_ARGS__)
}  // namespace sponge::logging

#define SPONGE_TRACE(...) \
    SPDLOG_LOGGER_TRACE(sponge::logging::Log::getAppLogger(), __VA_ARGS__)
#define SPONGE_DEBUG(...) \
    SPDLOG_LOGGER_DEBUG(sponge::logging::Log::getAppLogger(), __VA_ARGS__)
#define SPONGE_INFO(...) \
    SPDLOG_LOGGER_INFO(sponge::logging::Log::getAppLogger(), __VA_ARGS__)
#define SPONGE_WARN(...) \
    SPDLOG_LOGGER_WARN(sponge::logging::Log::getAppLogger(), __VA_ARGS__)
#define SPONGE_ERROR(...) \
    SPDLOG_LOGGER_ERROR(sponge::logging::Log::getAppLogger(), __VA_ARGS__)
#define SPONGE_CRITICAL(...) \
    SPDLOG_LOGGER_CRITICAL(sponge::logging::Log::getAppLogger(), __VA_ARGS__)
