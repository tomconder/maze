#pragma once

#include <cstddef>
#include <functional>

namespace assetconv {

// How many threads parallelFor may use. Set once at startup, from --threads.
void setThreadCount(unsigned threads);

// Calls work(i) once for every i in [0, count), spread over up to
// setThreadCount() threads, and returns when all have finished. Indices are
// taken in order, so each call must not depend on another. With one thread or
// one index it runs on the calling thread.
void parallelFor(size_t count, const std::function<void(size_t)>& work);

}  // namespace assetconv
