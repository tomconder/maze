# Custom Allocator Design: mimalloc + Tracy

**Date:** 2026-05-23
**Branch:** add-gamepad-support (or a dedicated branch)
**Goal:** Replace the default system allocator with Microsoft's mimalloc for performance, and feed every heap event into Tracy for profiling visibility.

***

## Motivation

The default CRT allocator (`malloc`/`operator new`) is a general-purpose allocator with no awareness of the render-thread / update-thread workload pattern in this engine. mimalloc uses thread-local segment caches that reduce lock contention and fragmentation under multi-threaded allocation patterns. Tracy integration gives us a live memory view in the profiler at zero cost when profiling is off.

***

## Scope

**Global drop-in replacement** for all `operator new` / `operator delete` calls in the process. This covers every C++ heap allocation: STL containers, smart pointers, spdlog, nlohmann\_json, and all third-party libraries linked statically.

**Out of scope:**

* Per-subsystem arena or pool allocators (future work if hot-path profiling reveals specific bottlenecks)
* `malloc`/`free` interception on Windows (CRT DLL boundary prevents this without the redirect DLL; C-style allocations from any DLL dependency are not covered — not a real concern for this statically-linked project)

***

## Architecture

```
vcpkg.json          → adds mimalloc dependency
game/CMakeLists.txt → find_package + link mimalloc-static (executable only)
sponge/src/debug/profiler.hpp → adds SPONGE_PROFILE_ALLOC / SPONGE_PROFILE_FREE macros
game/src/mimalloc_override.cpp → all operator new/delete variants; calls mi_malloc + Tracy
```

The override lives in the `game` executable target, not the `sponge` library. The sponge library remains allocator-agnostic so future consumers can make their own choice.

***

## File Changes

### 1. `vcpkg.json`

Add to the `dependencies` array:

```json
{
  "name": "mimalloc",
  "version>=": "2.1.7"
}
```

Declare this explicitly even though mimalloc is already a transitive dependency of glfw3 in vcpkg. Relying on transitive availability is fragile — if the glfw port ever changes its dependency tree, the build would silently break or pick up a different version. An explicit entry pins the version and makes the intent clear.

### 2. `game/CMakeLists.txt`

Add after the existing `find_package` block:

```cmake
find_package(mimalloc CONFIG REQUIRED)
```

Add to the `target_link_libraries` block:

```cmake
target_link_libraries(game PRIVATE mimalloc-static)
```

The new file `game/src/mimalloc_override.cpp` is picked up automatically by the existing `file(GLOB SOURCE_FILES ... src/*.cpp ...)` glob — no further CMake changes needed.

### 3. `sponge/src/debug/profiler.hpp`

Add two macros following the existing `SPONGE_PROFILE_*` pattern:

```cpp
#ifdef ENABLE_PROFILING
#define SPONGE_PROFILE_ALLOC(ptr, size) TracyAlloc(ptr, size)
#define SPONGE_PROFILE_FREE(ptr)        TracyFree(ptr)
#else
#define SPONGE_PROFILE_ALLOC(ptr, size)
#define SPONGE_PROFILE_FREE(ptr)
#endif
```

Both macros compile to nothing when `ENABLE_PROFILING` is off — zero overhead in non-profiling builds.

### 4. `game/src/mimalloc_override.cpp` (new file)

Overrides all C++17/23 `operator new` / `operator delete` variants. Each allocation calls `mi_malloc` (or `mi_malloc_aligned`) and then `SPONGE_PROFILE_ALLOC`; each deallocation calls `SPONGE_PROFILE_FREE` then `mi_free`.

```cpp
#include <mimalloc.h>
#include "debug/profiler.hpp"
#include <new>

void* operator new(std::size_t n) {
    auto* p = mi_malloc(n);
    if (!p) throw std::bad_alloc{};
    SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new[](std::size_t n) {
    auto* p = mi_malloc(n);
    if (!p) throw std::bad_alloc{};
    SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new(std::size_t n, std::nothrow_t const&) noexcept {
    auto* p = mi_malloc(n);
    if (p) SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new[](std::size_t n, std::nothrow_t const&) noexcept {
    auto* p = mi_malloc(n);
    if (p) SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new(std::size_t n, std::align_val_t al) {
    auto* p = mi_malloc_aligned(n, static_cast<std::size_t>(al));
    if (!p) throw std::bad_alloc{};
    SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new[](std::size_t n, std::align_val_t al) {
    auto* p = mi_malloc_aligned(n, static_cast<std::size_t>(al));
    if (!p) throw std::bad_alloc{};
    SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new(std::size_t n, std::align_val_t al, std::nothrow_t const&) noexcept {
    auto* p = mi_malloc_aligned(n, static_cast<std::size_t>(al));
    if (p) SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new[](std::size_t n, std::align_val_t al, std::nothrow_t const&) noexcept {
    auto* p = mi_malloc_aligned(n, static_cast<std::size_t>(al));
    if (p) SPONGE_PROFILE_ALLOC(p, n);
    return p;
}

void operator delete(void* p) noexcept                           { SPONGE_PROFILE_FREE(p); mi_free(p); }
void operator delete[](void* p) noexcept                         { SPONGE_PROFILE_FREE(p); mi_free(p); }
void operator delete(void* p, std::nothrow_t const&) noexcept    { SPONGE_PROFILE_FREE(p); mi_free(p); }
void operator delete[](void* p, std::nothrow_t const&) noexcept  { SPONGE_PROFILE_FREE(p); mi_free(p); }
void operator delete(void* p, std::size_t) noexcept              { SPONGE_PROFILE_FREE(p); mi_free(p); }
void operator delete[](void* p, std::size_t) noexcept            { SPONGE_PROFILE_FREE(p); mi_free(p); }
void operator delete(void* p, std::align_val_t) noexcept         { SPONGE_PROFILE_FREE(p); mi_free(p); }
void operator delete[](void* p, std::align_val_t) noexcept       { SPONGE_PROFILE_FREE(p); mi_free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept  { SPONGE_PROFILE_FREE(p); mi_free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { SPONGE_PROFILE_FREE(p); mi_free(p); }
```

**Tracy on-demand mode:** Tracy is configured with `on-demand` in vcpkg.json. Early allocations before the profiler connects are buffered by Tracy and sent on connection — no startup ordering issue.

***

## What Is and Is Not Covered

| Allocation type | Covered | Notes |
|---|---|---|
| `operator new` / `delete` (all variants) | Yes | The override TU handles all 18 standard signatures |
| STL containers, smart pointers | Yes | All use `operator new` internally |
| spdlog, nlohmann\_json, meshoptimizer | Yes | Statically linked, use `operator new` |
| Tracy profiler visibility | Yes | When `ENABLE_PROFILING` is set |
| `malloc`/`free` in C code (Windows DLLs) | No | CRT DLL boundary; not relevant for this static build |

***

## Testing

1. Build with and without `ENABLE_PROFILING` to verify both paths compile cleanly.
2. Connect Tracy profiler — the Memory tab should show allocations with source attribution.
3. Run a profiling session before and after to verify reduced allocation overhead in hot loops.
4. Call `mi_stats_print(nullptr)` at shutdown and verify plausible allocation counts (optional sanity check; remove before merging).
