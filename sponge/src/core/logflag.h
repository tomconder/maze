#ifndef MAZE_LOGFLAG_H
#define MAZE_LOGFLAG_H

#include <spdlog/pattern_formatter.h>

class LogFlag : public spdlog::custom_flag_formatter
{
  public:
    void format(const spdlog::details::log_msg &msg, const std::tm &tm_time, spdlog::memory_buf_t &dest) override;
    std::unique_ptr<custom_flag_formatter> clone() const override;
};

#if !defined(LOGFLAG_LEVEL_NAMES)
#define LOGFLAG_LEVEL_NAME_TRACE    spdlog::string_view_t("TRACE   ", 8)
#define LOGFLAG_LEVEL_NAME_DEBUG    spdlog::string_view_t("DEBUG   ", 8)
#define LOGFLAG_LEVEL_NAME_INFO     spdlog::string_view_t("INFO    ", 8)
#define LOGFLAG_LEVEL_NAME_WARNING  spdlog::string_view_t("WARNING ", 8)
#define LOGFLAG_LEVEL_NAME_ERROR    spdlog::string_view_t("ERROR   ", 8)
#define LOGFLAG_LEVEL_NAME_CRITICAL spdlog::string_view_t("CRITICAL", 8)
#define LOGFLAG_LEVEL_NAME_OFF      spdlog::string_view_t("OFF     ", 8)

#define LOGFLAG_LEVEL_NAMES                                                                                            \
    {                                                                                                                  \
        LOGFLAG_LEVEL_NAME_TRACE, LOGFLAG_LEVEL_NAME_DEBUG, LOGFLAG_LEVEL_NAME_INFO, LOGFLAG_LEVEL_NAME_WARNING,       \
            LOGFLAG_LEVEL_NAME_ERROR, LOGFLAG_LEVEL_NAME_CRITICAL, LOGFLAG_LEVEL_NAME_OFF                              \
    }
#endif

#endif // MAZE_LOGFLAG_H
