#pragma once

// spdlog.h is slow to compile, so only log.cpp and the log sinks include it
#include <fmt/base.h>

#include <memory>
#include <string>

namespace spdlog::sinks {
class sink;
}  // namespace spdlog::sinks

namespace sponge::logging {
enum class Channel { App, Core, Gl };

// values match spdlog::level::level_enum and SPDLOG_ACTIVE_LEVEL
enum class Level { Trace, Debug, Info, Warn, Error, Critical };

struct LogItem {
    std::string message;
    std::string loggerName;
    Level       level;
};

class Log {
public:
    static void init(const std::string& logfile);

    static void shutdown();

    static void addSink(const std::shared_ptr<spdlog::sinks::sink>& sink,
                        const std::string&                          pattern);

    template <typename... Args>
    static void print(const Channel channel, const Level level,
                      const char* file, const int line, const char* function,
                      fmt::format_string<Args...> format, Args&&... args) {
        write(channel, level, file, line, function, format.get(),
              fmt::make_format_args(args...));
    }

    static constexpr const char* colorFormatPattern =
        "%^%L%m%d %T.%f %7t %s:%# [%n] %v%$";
    static constexpr const char* fileFormatPattern =
        "%L%m%d %T.%f %7t %s:%# [%n] %v";

private:
    static void write(Channel channel, Level level, const char* file, int line,
                      const char* function, fmt::string_view format,
                      fmt::format_args args);
};
}  // namespace sponge::logging

#define SPONGE_LOG(channel, level, ...)                                       \
    ::sponge::logging::Log::print(                                            \
        ::sponge::logging::Channel::channel, ::sponge::logging::Level::level, \
        __FILE__, __LINE__, static_cast<const char*>(__FUNCTION__),           \
        __VA_ARGS__)

// Levels below SPDLOG_ACTIVE_LEVEL compile to nothing. It must be a number:
// the preprocessor reads an spdlog level name as 0 without spdlog/common.h.
static_assert(SPDLOG_ACTIVE_LEVEL >= 0 && SPDLOG_ACTIVE_LEVEL <= 6);

#if SPDLOG_ACTIVE_LEVEL <= 0
#define SPONGE_LOG_TRACE(channel, ...) SPONGE_LOG(channel, Trace, __VA_ARGS__)
#else
#define SPONGE_LOG_TRACE(channel, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= 1
#define SPONGE_LOG_DEBUG(channel, ...) SPONGE_LOG(channel, Debug, __VA_ARGS__)
#else
#define SPONGE_LOG_DEBUG(channel, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= 2
#define SPONGE_LOG_INFO(channel, ...) SPONGE_LOG(channel, Info, __VA_ARGS__)
#else
#define SPONGE_LOG_INFO(channel, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= 3
#define SPONGE_LOG_WARN(channel, ...) SPONGE_LOG(channel, Warn, __VA_ARGS__)
#else
#define SPONGE_LOG_WARN(channel, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= 4
#define SPONGE_LOG_ERROR(channel, ...) SPONGE_LOG(channel, Error, __VA_ARGS__)
#else
#define SPONGE_LOG_ERROR(channel, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= 5
#define SPONGE_LOG_CRITICAL(channel, ...) \
    SPONGE_LOG(channel, Critical, __VA_ARGS__)
#else
#define SPONGE_LOG_CRITICAL(channel, ...) (void)0
#endif

#define SPONGE_CORE_TRACE(...)    SPONGE_LOG_TRACE(Core, __VA_ARGS__)
#define SPONGE_CORE_DEBUG(...)    SPONGE_LOG_DEBUG(Core, __VA_ARGS__)
#define SPONGE_CORE_INFO(...)     SPONGE_LOG_INFO(Core, __VA_ARGS__)
#define SPONGE_CORE_WARN(...)     SPONGE_LOG_WARN(Core, __VA_ARGS__)
#define SPONGE_CORE_ERROR(...)    SPONGE_LOG_ERROR(Core, __VA_ARGS__)
#define SPONGE_CORE_CRITICAL(...) SPONGE_LOG_CRITICAL(Core, __VA_ARGS__)

#define SPONGE_GL_TRACE(...)    SPONGE_LOG_TRACE(Gl, __VA_ARGS__)
#define SPONGE_GL_DEBUG(...)    SPONGE_LOG_DEBUG(Gl, __VA_ARGS__)
#define SPONGE_GL_INFO(...)     SPONGE_LOG_INFO(Gl, __VA_ARGS__)
#define SPONGE_GL_WARN(...)     SPONGE_LOG_WARN(Gl, __VA_ARGS__)
#define SPONGE_GL_ERROR(...)    SPONGE_LOG_ERROR(Gl, __VA_ARGS__)
#define SPONGE_GL_CRITICAL(...) SPONGE_LOG_CRITICAL(Gl, __VA_ARGS__)

#define SPONGE_TRACE(...)    SPONGE_LOG_TRACE(App, __VA_ARGS__)
#define SPONGE_DEBUG(...)    SPONGE_LOG_DEBUG(App, __VA_ARGS__)
#define SPONGE_INFO(...)     SPONGE_LOG_INFO(App, __VA_ARGS__)
#define SPONGE_WARN(...)     SPONGE_LOG_WARN(App, __VA_ARGS__)
#define SPONGE_ERROR(...)    SPONGE_LOG_ERROR(App, __VA_ARGS__)
#define SPONGE_CRITICAL(...) SPONGE_LOG_CRITICAL(App, __VA_ARGS__)
