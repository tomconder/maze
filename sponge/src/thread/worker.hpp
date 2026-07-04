#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace sponge::thread {

// Generic worker thread: the owner calls kick() to run a task and
// waitForComplete() to synchronize before the next kick. Used for both the
// update worker (game logic, no GL) and the render worker (owns the GL
// context); kicking both in the same loop iteration overlaps update[N] with
// render[N].
class Worker {
public:
    ~Worker();

    void start();
    void kick(std::function<bool()> newTask);
    bool waitForComplete();
    void stop();

private:
    std::thread             thread;
    std::mutex              mutex;
    std::condition_variable cv;

    std::function<bool()> task;
    bool                  hasWork    = false;
    bool                  done       = true;
    bool                  result     = true;
    bool                  shouldStop = false;
};

}  // namespace sponge::thread
