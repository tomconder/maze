#pragma once

#ifndef SPDLOG_H
#if !NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#endif
#include "spdlog/spdlog.h"
#endif

static const char *const SPONGE_LOG_FILE = "log.txt";

namespace Sponge {

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

    static std::shared_ptr<spdlog::logger> &getGlLogger() {
        return glLogger;
    }

   private:
    static std::shared_ptr<spdlog::logger> appLogger;
    static std::shared_ptr<spdlog::logger> coreLogger;
    static std::shared_ptr<spdlog::logger> glLogger;
};

#define SPONGE_CORE_TRACE(...)    SPDLOG_LOGGER_TRACE(Log::getCoreLogger(), __VA_ARGS__)
#define SPONGE_CORE_DEBUG(...)    SPDLOG_LOGGER_DEBUG(Log::getCoreLogger(), __VA_ARGS__)
#define SPONGE_CORE_INFO(...)     SPDLOG_LOGGER_INFO(Log::getCoreLogger(), __VA_ARGS__)
#define SPONGE_CORE_WARN(...)     SPDLOG_LOGGER_WARN(Log::getCoreLogger(), __VA_ARGS__)
#define SPONGE_CORE_ERROR(...)    SPDLOG_LOGGER_ERROR(Log::getCoreLogger(), __VA_ARGS__)
#define SPONGE_CORE_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(Log::getCoreLogger(), __VA_ARGS__)

#define SPONGE_GL_TRACE(...)    SPDLOG_LOGGER_TRACE(Log::getGlLogger(), __VA_ARGS__)
#define SPONGE_GL_DEBUG(...)    SPDLOG_LOGGER_DEBUG(Log::getGlLogger(), __VA_ARGS__)
#define SPONGE_GL_INFO(...)     SPDLOG_LOGGER_INFO(Log::getGlLogger(), __VA_ARGS__)
#define SPONGE_GL_WARN(...)     SPDLOG_LOGGER_WARN(Log::getGlLogger(), __VA_ARGS__)
#define SPONGE_GL_ERROR(...)    SPDLOG_LOGGER_ERROR(Log::getGlLogger(), __VA_ARGS__)
#define SPONGE_GL_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(Log::getGlLogger(), __VA_ARGS__)

}  // namespace Sponge

#define SPONGE_TRACE(...)    SPDLOG_LOGGER_TRACE(Sponge::Log::getAppLogger(), __VA_ARGS__)
#define SPONGE_DEBUG(...)    SPDLOG_LOGGER_DEBUG(Sponge::Log::getAppLogger(), __VA_ARGS__)
#define SPONGE_INFO(...)     SPDLOG_LOGGER_INFO(Sponge::Log::getAppLogger(), __VA_ARGS__)
#define SPONGE_WARN(...)     SPDLOG_LOGGER_WARN(Sponge::Log::getAppLogger(), __VA_ARGS__)
#define SPONGE_ERROR(...)    SPDLOG_LOGGER_ERROR(Sponge::Log::getAppLogger(), __VA_ARGS__)
#define SPONGE_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(Sponge::Log::getAppLogger(), __VA_ARGS__)
