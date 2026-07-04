#include "worker.hpp"

namespace sponge::thread {

Worker::~Worker() {
    if (thread.joinable()) {
        stop();
    }
}

// Start the worker thread. Must be called before kick().
void Worker::start() {
    thread = std::thread([this] {
        while (true) {
            std::function<bool()> currentTask;

            {
                std::unique_lock lock(mutex);
                // Predicate form guards against spurious wakeups
                cv.wait(lock, [this] { return hasWork || shouldStop; });

                if (shouldStop) {
                    return;
                }

                currentTask = std::move(task);
                hasWork     = false;
            }

            const bool taskResult = currentTask();

            {
                std::scoped_lock lock(mutex);
                result = taskResult;
                done   = true;
            }
            cv.notify_all();
        }
    });
}

// Assign work and wake the thread. Returns immediately.
// The task returns false when the game loop should terminate.
void Worker::kick(std::function<bool()> newTask) {
    {
        std::scoped_lock lock(mutex);
        task    = std::move(newTask);
        done    = false;
        hasWork = true;
    }
    cv.notify_one();
}

// Block until the current task completes. Returns the task's return value.
// Returns immediately if no task was kicked since the last wait.
bool Worker::waitForComplete() {
    std::unique_lock lock(mutex);
    cv.wait(lock, [this] { return done || shouldStop; });
    return result;
}

// Signal the thread to exit and join it.
void Worker::stop() {
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
