#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace sponge::thread {

// Render thread state machine: Idle → Kick → Busy → Idle
// The render thread owns the OpenGL context and executes all GPU commands.
// The main/update thread calls Kick() to begin a frame and
// BlockUntilRenderComplete() to synchronize before the next submission.
class RenderThread {
public:
    enum class State { Idle, Kick, Busy };

    ~RenderThread();

    void start(std::function<void()> renderFunc);
    void kick();
    void blockUntilRenderComplete();
    void stop();
    bool isRunning() const;

private:
    std::thread             thread;
    mutable std::mutex      mutex;
    std::condition_variable cv;
    State                   state      = State::Idle;
    bool                    shouldStop = false;
};

}  // namespace sponge::thread
