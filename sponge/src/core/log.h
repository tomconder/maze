#pragma once

#include <spdlog/spdlog.h>

static const char *const SPONGE_LOG_FILE = "log.txt";

class Log {
   public:
    static void init();
    static void shutdown() {
        spdlog::shutdown();
    }

    static std::shared_ptr<spdlog::logger> &getAppLogger() {
        return appLogger;
    }
    static std::shared_ptr<spdlog::logger> &getCoreLogger() {
        return coreLogger;
    }

   private:
    static std::shared_ptr<spdlog::logger> coreLogger;
    static std::shared_ptr<spdlog::logger> appLogger;
};

#define SPONGE_CORE_TRACE(...)    SPDLOG_LOGGER_TRACE(Log::getCoreLogger(), __VA_ARGS__)
#define SPONGE_CORE_INFO(...)     SPDLOG_LOGGER_INFO(Log::getCoreLogger(), __VA_ARGS__)
#define SPONGE_CORE_WARN(...)     SPDLOG_LOGGER_WARN(Log::getCoreLogger(), __VA_ARGS__)
#define SPONGE_CORE_ERROR(...)    SPDLOG_LOGGER_ERROR(Log::getCoreLogger(), __VA_ARGS__)
#define SPONGE_CORE_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(Log::getCoreLogger(), __VA_ARGS__)

#define SPONGE_TRACE(...)    SPDLOG_LOGGER_TRACE(Log::getAppLogger(), __VA_ARGS__)
#define SPONGE_INFO(...)     SPDLOG_LOGGER_INFO(Log::getAppLogger(), __VA_ARGS__)
#define SPONGE_WARN(...)     SPDLOG_LOGGER_WARN(Log::getAppLogger(), __VA_ARGS__)
#define SPONGE_ERROR(...)    SPDLOG_LOGGER_ERROR(Log::getAppLogger(), __VA_ARGS__)
#define SPONGE_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(Log::getAppLogger(), __VA_ARGS__)
