#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace sponge::thread {

// One of two game-update worker threads. Each thread waits for a kick from
// the main/GLFW thread, runs the assigned task (pure game logic, no GL),
// then signals completion so the main thread can kick the render thread.
//
// The two UpdateThread instances are used in a ping-pong pattern:
//   Frame N   → UpdateThread[N % 2] runs game logic → kicks render
//   Frame N+1 → UpdateThread[(N+1) % 2] starts while render is still busy
//
// This creates one frame of pipeline overlap: update[N+1] runs in parallel
// with render[N], giving full CPU utilization without data races because each
// update thread writes to its own isolated snapshot slot.
class UpdateThread {
public:
    ~UpdateThread() {
        if (thread.joinable()) {
            stop();
        }
    }

    // Start the worker thread. Must be called before kick().
    void start() {
        thread = std::thread([this] {
            while (true) {
                std::function<bool(double)> task;
                double                      elapsed = 0.0;

                {
                    std::unique_lock lock(mutex);
                    cv.wait(lock, [this] { return hasWork || shouldStop; });

                    if (shouldStop) {
                        return;
                    }

                    task      = std::move(this->task);
                    elapsed   = elapsedTime;
                    hasWork   = false;
                }

                const bool result = task(elapsed);

                {
                    std::lock_guard lock(mutex);
                    this->result = result;
                    done         = true;
                }
                cv.notify_all();
            }
        });
    }

    // Assign work and wake the thread. Non-blocking — returns immediately.
    // task(elapsedTime) returns false when the game loop should terminate.
    void kick(double elapsed, std::function<bool(double)> newTask) {
        {
            std::lock_guard lock(mutex);
            elapsedTime = elapsed;
            task        = std::move(newTask);
            done        = false;
            hasWork     = true;
        }
        cv.notify_one();
    }

    // Block until the current task completes. Returns the task's return value.
    bool waitForComplete() {
        std::unique_lock lock(mutex);
        cv.wait(lock, [this] { return done || shouldStop; });
        return result;
    }

    // Signal the thread to exit and join it.
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

private:
    std::thread             thread;
    std::mutex              mutex;
    std::condition_variable cv;

    std::function<bool(double)> task;
    double                      elapsedTime = 0.0;
    bool                        hasWork     = false;
    bool                        done        = true;
    bool                        result      = true;
    bool                        shouldStop  = false;
};

}  // namespace sponge::thread
