#pragma once

#include "platform/sdl/engine.hpp"
#include <spdlog/sinks/base_sink.h>

namespace sponge::imgui {

template <typename Mutex>
class ImguiSink final : public spdlog::sinks::base_sink<Mutex> {
   public:
    ImguiSink() = default;

   protected:
    void sink_it_(const spdlog::details::log_msg& msg) override;
    void flush_() override {}
};

template <typename Mutex>
void ImguiSink<Mutex>::sink_it_(const spdlog::details::log_msg& msg) {
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    const auto formattedText = fmt::to_string(formatted);

    const LogItem it{ formattedText, msg.logger_name.data(), msg.level };

    platform::sdl::Engine::get().addMessage(it);
}

}  // namespace sponge::imgui
