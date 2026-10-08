#include "parallel.hpp"

#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>

namespace {
unsigned threadCount = 1;
}  // namespace

namespace assetconv {

void setThreadCount(const unsigned threads) {
    threadCount = std::max(threads, 1U);
}

void parallelFor(const size_t count, const std::function<void(size_t)>& work) {
    const auto threads = std::min<size_t>(threadCount, count);
    if (threads <= 1) {
        for (size_t i = 0; i < count; i++) {
            work(i);
        }
        return;
    }

    std::atomic<size_t>       next{ 0 };
    std::vector<std::jthread> workers;
    for (size_t t = 0; t < threads; t++) {
        workers.emplace_back([&] {
            for (auto i = next.fetch_add(1); i < count; i = next.fetch_add(1)) {
                work(i);
            }
        });
    }
    workers.clear();  // joins
}

}  // namespace assetconv
