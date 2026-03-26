#include "renderthread.hpp"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace sponge::thread {

RenderThread::~RenderThread() {
    if (thread.joinable()) {
        stop();
    }
}

// Start the render thread. renderFunc is called every frame.
void RenderThread::start(std::function<void()> renderFunc) {
    thread = std::thread([this, renderFunc = std::move(renderFunc)] {
        while (true) {
            {
                std::unique_lock lock(mutex);
                // Predicate form guards against spurious wakeups on Linux
                cv.wait(lock,
                        [this] { return state == State::Kick || shouldStop; });
                if (shouldStop) {
                    return;
                }
                state = State::Busy;
            }

            renderFunc();

            {
                std::scoped_lock lock(mutex);
                state = State::Idle;
            }
            cv.notify_all();
        }
    });
}

// Called by the main thread to wake the render thread for frame N.
void RenderThread::kick() {
    {
        std::scoped_lock lock(mutex);
        state = State::Kick;
    }
    cv.notify_one();
}

// Called by the main thread to wait for frame N to finish before
// swapping command queues and kicking the next frame.
void RenderThread::blockUntilRenderComplete() {
    std::unique_lock lock(mutex);
    cv.wait(lock, [this] { return state == State::Idle; });
}

// Signal the render thread to exit and wait for it to join.
void RenderThread::stop() {
    {
        std::scoped_lock lock(mutex);
        shouldStop = true;
    }
    cv.notify_all();
    if (thread.joinable()) {
        thread.join();
    }
}

bool RenderThread::isRunning() const {
    std::scoped_lock lock(mutex);
    return state != State::Idle || !shouldStop;
}

}  // namespace sponge::thread
