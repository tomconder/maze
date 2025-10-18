#pragma once

#include "logging/log.hpp"
#include "platform/glfw/core/application.hpp"

#include <spdlog/sinks/base_sink.h>

namespace sponge::platform::glfw::imgui {

using logging::LogItem;

template <typename Mutex>
class Sink final : public spdlog::sinks::base_sink<Mutex> {
public:
    Sink() = default;

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override;
    void flush_() override {}
};

template <typename Mutex>
void Sink<Mutex>::sink_it_(const spdlog::details::log_msg& msg) {
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    const auto formattedText = fmt::to_string(formatted);

    const LogItem it{ .message = formattedText,
                      .loggerName = msg.logger_name.data(),
                      .level = msg.level };

    core::Application::get().addMessage(it);
}

}  // namespace sponge::platform::glfw::imgui
