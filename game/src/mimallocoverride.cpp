#include <mimalloc.h>

#include "debug/profiler.hpp"

#include <new>

void* operator new(std::size_t n) {
    auto* p = mi_malloc(n);
    if (!p)
        throw std::bad_alloc{};
    SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new[](std::size_t n) {
    auto* p = mi_malloc(n);
    if (!p)
        throw std::bad_alloc{};
    SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new(std::size_t n, std::nothrow_t const&) noexcept {
    auto* p = mi_malloc(n);
    if (p)
        SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new[](std::size_t n, std::nothrow_t const&) noexcept {
    auto* p = mi_malloc(n);
    if (p)
        SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new(std::size_t n, std::align_val_t al) {
    auto* p = mi_malloc_aligned(n, static_cast<std::size_t>(al));
    if (!p)
        throw std::bad_alloc{};
    SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new[](std::size_t n, std::align_val_t al) {
    auto* p = mi_malloc_aligned(n, static_cast<std::size_t>(al));
    if (!p)
        throw std::bad_alloc{};
    SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new(std::size_t n, std::align_val_t al,
                   std::nothrow_t const&) noexcept {
    auto* p = mi_malloc_aligned(n, static_cast<std::size_t>(al));
    if (p)
        SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new[](std::size_t n, std::align_val_t al,
                     std::nothrow_t const&) noexcept {
    auto* p = mi_malloc_aligned(n, static_cast<std::size_t>(al));
    if (p)
        SPONGE_PROFILE_ALLOC(p, n);
    return p;
}

void operator delete(void* p) noexcept {
    SPONGE_PROFILE_FREE(p);
    mi_free(p);
}
void operator delete[](void* p) noexcept {
    SPONGE_PROFILE_FREE(p);
    mi_free(p);
}
void operator delete(void* p, std::nothrow_t const&) noexcept {
    SPONGE_PROFILE_FREE(p);
    mi_free(p);
}
void operator delete[](void* p, std::nothrow_t const&) noexcept {
    SPONGE_PROFILE_FREE(p);
    mi_free(p);
}
void operator delete(void* p, std::size_t) noexcept {
    SPONGE_PROFILE_FREE(p);
    mi_free(p);
}
void operator delete[](void* p, std::size_t) noexcept {
    SPONGE_PROFILE_FREE(p);
    mi_free(p);
}
void operator delete(void* p, std::align_val_t) noexcept {
    SPONGE_PROFILE_FREE(p);
    mi_free(p);
}
void operator delete[](void* p, std::align_val_t) noexcept {
    SPONGE_PROFILE_FREE(p);
    mi_free(p);
}
void operator delete(void* p, std::size_t, std::align_val_t) noexcept {
    SPONGE_PROFILE_FREE(p);
    mi_free(p);
}
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept {
    SPONGE_PROFILE_FREE(p);
    mi_free(p);
}
