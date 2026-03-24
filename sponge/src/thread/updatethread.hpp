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
    ~UpdateThread();

    void start();
    void kick(double elapsed, std::function<bool(double)> newTask);
    bool waitForComplete();
    void stop();

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
