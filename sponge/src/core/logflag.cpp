#include "logflag.h"

static spdlog::string_view_t level_string_views[] LOGFLAG_LEVEL_NAMES;

void LogFlag::format(const spdlog::details::log_msg &msg, const std::tm &tm_time, spdlog::memory_buf_t &dest) {
    auto level = level_string_views[msg.level];
    dest.append(level.data(), level.data() + level.size());
}

std::unique_ptr<spdlog::custom_flag_formatter> LogFlag::clone() const {
    return spdlog::details::make_unique<LogFlag>();
}