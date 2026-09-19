// Global operator new/delete that report every heap event to Tracy. Only
// profiling builds replace the operators; other builds use the standard ones.
#ifdef ENABLE_PROFILING

#include "debug/profiler.hpp"

#include <cstddef>
#include <cstdlib>
#include <new>

namespace {

void* allocate(const std::size_t n) noexcept {
    // malloc(0) may return nullptr, but new must return a unique pointer.
    auto* p = std::malloc(n != 0 ? n : 1);
    if (p) {
        SPONGE_PROFILE_ALLOC(p, n);
    }
    return p;
}

void* allocateAligned(const std::size_t n, const std::align_val_t al) noexcept {
    const auto alignment = static_cast<std::size_t>(al);
#ifdef _WIN32
    auto* p = _aligned_malloc(n != 0 ? n : 1, alignment);
#else
    // aligned_alloc requires the size to be a multiple of the alignment.
    const auto size =
        ((n != 0 ? n : 1) + alignment - 1) / alignment * alignment;
    auto* p = std::aligned_alloc(alignment, size);
#endif
    if (p) {
        SPONGE_PROFILE_ALLOC(p, n);
    }
    return p;
}

void release(void* p) noexcept {
    SPONGE_PROFILE_FREE(p);
    std::free(p);
}

void releaseAligned(void* p) noexcept {
    SPONGE_PROFILE_FREE(p);
#ifdef _WIN32
    _aligned_free(p);
#else
    std::free(p);
#endif
}

}  // namespace

// Replacement operators cannot match the parameter names the platform headers
// declare, and those names differ per standard library.
// NOLINTBEGIN(readability-inconsistent-declaration-parameter-name)
void* operator new(std::size_t n) {
    if (auto* p = allocate(n)) {
        return p;
    }
    throw std::bad_alloc{};
}
void* operator new[](std::size_t n) {
    if (auto* p = allocate(n)) {
        return p;
    }
    throw std::bad_alloc{};
}
void* operator new(std::size_t n, std::nothrow_t const&) noexcept {
    return allocate(n);
}
void* operator new[](std::size_t n, std::nothrow_t const&) noexcept {
    return allocate(n);
}
void* operator new(std::size_t n, std::align_val_t al) {
    if (auto* p = allocateAligned(n, al)) {
        return p;
    }
    throw std::bad_alloc{};
}
void* operator new[](std::size_t n, std::align_val_t al) {
    if (auto* p = allocateAligned(n, al)) {
        return p;
    }
    throw std::bad_alloc{};
}
void* operator new(std::size_t n, std::align_val_t al,
                   std::nothrow_t const&) noexcept {
    return allocateAligned(n, al);
}
void* operator new[](std::size_t n, std::align_val_t al,
                     std::nothrow_t const&) noexcept {
    return allocateAligned(n, al);
}

void operator delete(void* p) noexcept {
    release(p);
}
void operator delete[](void* p) noexcept {
    release(p);
}
void operator delete(void* p, std::nothrow_t const&) noexcept {
    release(p);
}
void operator delete[](void* p, std::nothrow_t const&) noexcept {
    release(p);
}
void operator delete(void* p, std::size_t) noexcept {
    release(p);
}
void operator delete[](void* p, std::size_t) noexcept {
    release(p);
}
void operator delete(void* p, std::align_val_t) noexcept {
    releaseAligned(p);
}
void operator delete[](void* p, std::align_val_t) noexcept {
    releaseAligned(p);
}
void operator delete(void* p, std::size_t, std::align_val_t) noexcept {
    releaseAligned(p);
}
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept {
    releaseAligned(p);
}
// NOLINTEND(readability-inconsistent-declaration-parameter-name)

#endif
