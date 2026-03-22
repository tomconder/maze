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

    ~RenderThread() {
        if (thread.joinable()) {
            stop();
        }
    }

    // Start the render thread. renderFunc is called every frame.
    void start(std::function<void()> renderFunc) {
        thread = std::thread([this, renderFunc = std::move(renderFunc)] {
            while (true) {
                {
                    std::unique_lock lock(mutex);
                    // Predicate form guards against spurious wakeups on Linux
                    cv.wait(lock, [this] {
                        return state == State::Kick || shouldStop;
                    });
                    if (shouldStop) {
                        return;
                    }
                    state = State::Busy;
                }

                renderFunc();

                {
                    std::lock_guard lock(mutex);
                    state = State::Idle;
                }
                cv.notify_all();
            }
        });
    }

    // Called by the main thread to wake the render thread for frame N.
    void kick() {
        {
            std::lock_guard lock(mutex);
            state = State::Kick;
        }
        cv.notify_one();
    }

    // Called by the main thread to wait for frame N to finish before
    // swapping command queues and kicking the next frame.
    void blockUntilRenderComplete() {
        std::unique_lock lock(mutex);
        cv.wait(lock, [this] { return state == State::Idle; });
    }

    // Signal the render thread to exit and wait for it to join.
    void stop() {
        {
            std::lock_guard lock(mutex);
            shouldStop = true;
        }
        cv.notify_all();
        if (thread.joinable()) {
            thread.join();
        }
    }

    bool isRunning() const {
        std::lock_guard lock(mutex);
        return state != State::Idle || !shouldStop;
    }

private:
    std::thread             thread;
    mutable std::mutex      mutex;
    std::condition_variable cv;
    State                   state      = State::Idle;
    bool                    shouldStop = false;
};

}  // namespace sponge::thread
