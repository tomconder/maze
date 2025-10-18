#pragma once

#include <spdlog/pattern_formatter.h>

#include <memory>

namespace sponge {

class LogFlag final : public spdlog::custom_flag_formatter {
public:
    void format(const spdlog::details::log_msg& msg, const std::tm& tm_time,
                spdlog::memory_buf_t& dest) override;
    std::unique_ptr<custom_flag_formatter> clone() const override;
};

}  // namespace sponge

#if !defined(LOGFLAG_LEVEL_NAMES)
#define LOGFLAG_LEVEL_NAME_TRACE    spdlog::string_view_t("T", 1)
#define LOGFLAG_LEVEL_NAME_DEBUG    spdlog::string_view_t("D", 1)
#define LOGFLAG_LEVEL_NAME_INFO     spdlog::string_view_t("I", 1)
#define LOGFLAG_LEVEL_NAME_WARNING  spdlog::string_view_t("W", 1)
#define LOGFLAG_LEVEL_NAME_ERROR    spdlog::string_view_t("E", 1)
#define LOGFLAG_LEVEL_NAME_CRITICAL spdlog::string_view_t("C", 1)
#define LOGFLAG_LEVEL_NAME_OFF      spdlog::string_view_t("O", 1)

#define LOGFLAG_LEVEL_NAMES                                  \
    { LOGFLAG_LEVEL_NAME_TRACE, LOGFLAG_LEVEL_NAME_DEBUG,    \
      LOGFLAG_LEVEL_NAME_INFO,  LOGFLAG_LEVEL_NAME_WARNING,  \
      LOGFLAG_LEVEL_NAME_ERROR, LOGFLAG_LEVEL_NAME_CRITICAL, \
      LOGFLAG_LEVEL_NAME_OFF }
#endif
