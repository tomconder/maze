#include "updatethread.hpp"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace sponge::thread {

UpdateThread::~UpdateThread() {
    if (thread.joinable()) {
        stop();
    }
}

// Start the worker thread. Must be called before kick().
void UpdateThread::start() {
    thread = std::thread([this] {
        while (true) {
            std::function<bool(double)> startTask;
            double                      elapsed = 0.0;

            {
                std::unique_lock lock(mutex);
                cv.wait(lock, [this] { return hasWork || shouldStop; });

                if (shouldStop) {
                    return;
                }

                startTask = std::move(this->task);
                elapsed   = elapsedTime;
                hasWork   = false;
            }

            {
                const bool       startResult = startTask(elapsed);
                std::scoped_lock lock(mutex);
                this->result = startResult;
                done         = true;
            }
            cv.notify_all();
        }
    });
}

// Assign work and wake the thread. Non-blocking — returns immediately.
// task(elapsedTime) returns false when the game loop should terminate.
void UpdateThread::kick(double elapsed, std::function<bool(double)> newTask) {
    {
        std::scoped_lock lock(mutex);
        elapsedTime = elapsed;
        task        = std::move(newTask);
        done        = false;
        hasWork     = true;
    }
    cv.notify_one();
}

// Block until the current task completes. Returns the task's return value.
bool UpdateThread::waitForComplete() {
    std::unique_lock lock(mutex);
    cv.wait(lock, [this] { return done || shouldStop; });
    return result;
}

// Signal the thread to exit and join it.
void UpdateThread::stop() {
    {
        std::scoped_lock lock(mutex);
        shouldStop = true;
    }
    cv.notify_all();
    if (thread.joinable()) {
        thread.join();
    }
}

}  // namespace sponge::thread
