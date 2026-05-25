#include <mimalloc.h>

#include "debug/profiler.hpp"

#include <new>

#if defined(__APPLE__)
// Apple's libc++ std::basic_string::__grow_by_and_replace calls system realloc
// directly rather than going through operator new/delete. With operator new
// overridden to use mimalloc, string buffers are mimalloc memory, but system
// realloc does not recognise mimalloc pointers and aborts.
//
// DYLD interposition does not work here: libc++'s call to realloc is resolved
// directly within the dyld shared cache, bypassing the stub mechanism that
// interposition relies on.
//
// The fix is to register mimalloc as a malloc zone. System realloc uses
// malloc_zone_from_ptr to find the owning zone for a pointer (querying each
// zone's size callback), then dispatches to that zone's realloc. Registering
// mimalloc as a zone makes system realloc correctly dispatch to mi_realloc for
// mimalloc-owned pointers, even when called from deep inside libc++.
//
// constructor(101) is the earliest user priority, running before any C++
// static initializers, ensuring the zone is in place before the first
// operator new allocates anything.
#include <malloc/malloc.h>

static size_t mi_zone_size(malloc_zone_t*, const void* p) noexcept {
    if (!mi_is_in_heap_region(p))
        return 0;
    return mi_usable_size(p);
}
static void* mi_zone_malloc(malloc_zone_t*, size_t n) noexcept {
    return mi_malloc(n);
}
static void* mi_zone_calloc(malloc_zone_t*, size_t c, size_t n) noexcept {
    return mi_calloc(c, n);
}
static void* mi_zone_valloc(malloc_zone_t*, size_t n) noexcept {
    return mi_malloc_aligned(n, 4096);
}
static void mi_zone_free(malloc_zone_t*, void* p) noexcept {
    mi_free(p);
}
static void* mi_zone_realloc(malloc_zone_t*, void* p, size_t n) noexcept {
    return mi_realloc(p, n);
}
static void mi_zone_destroy(malloc_zone_t* zone) noexcept {
    (void)zone;
}
static void* mi_zone_memalign(malloc_zone_t*, size_t al, size_t n) noexcept {
    return mi_malloc_aligned(n, al);
}
static void mi_zone_free_definite_size(malloc_zone_t*, void* p,
                                       size_t n) noexcept {
    mi_free_size(p, n);
}

static kern_return_t mi_enumerator(task_t, void*, unsigned, vm_address_t,
                                   memory_reader_t, vm_range_recorder_t) {
    return KERN_SUCCESS;
}
static size_t mi_good_size_fn(malloc_zone_t*, size_t n) {
    return mi_good_size(n);
}
static boolean_t mi_check_fn(malloc_zone_t* /*zone*/) {
    return true;
}
static void mi_print_fn(malloc_zone_t* /*zone*/, boolean_t) {}
static void mi_log_fn(malloc_zone_t* /*zone*/, void*) {}
static void mi_force_lock_fn(malloc_zone_t* /*zone*/) {}
static void mi_force_unlock_fn(malloc_zone_t* /*zone*/) {}
static void mi_statistics_fn(malloc_zone_t* /*zone*/, malloc_statistics_t* s) {
    s->blocks_in_use   = 0;
    s->size_in_use     = 0;
    s->max_size_in_use = 0;
    s->size_allocated  = 0;
}
static boolean_t mi_zone_locked_fn(malloc_zone_t* /*zone*/) {
    return false;
}

static malloc_introspection_t mi_introspect;
static malloc_zone_t          mi_zone;

__attribute__((constructor(101))) static void mi_register_zone() noexcept {
    mi_introspect.enumerator   = mi_enumerator;
    mi_introspect.good_size    = mi_good_size_fn;
    mi_introspect.check        = mi_check_fn;
    mi_introspect.print        = mi_print_fn;
    mi_introspect.log          = mi_log_fn;
    mi_introspect.force_lock   = mi_force_lock_fn;
    mi_introspect.force_unlock = mi_force_unlock_fn;
    mi_introspect.statistics   = mi_statistics_fn;
    mi_introspect.zone_locked  = mi_zone_locked_fn;

    mi_zone.size               = mi_zone_size;
    mi_zone.malloc             = mi_zone_malloc;
    mi_zone.calloc             = mi_zone_calloc;
    mi_zone.valloc             = mi_zone_valloc;
    mi_zone.free               = mi_zone_free;
    mi_zone.realloc            = mi_zone_realloc;
    mi_zone.destroy            = mi_zone_destroy;
    mi_zone.zone_name          = "mimalloc";
    mi_zone.introspect         = &mi_introspect;
    mi_zone.version            = 6;
    mi_zone.memalign           = mi_zone_memalign;
    mi_zone.free_definite_size = mi_zone_free_definite_size;

    malloc_zone_register(&mi_zone);
}
#endif

void* operator new(std::size_t n) {
    auto* p = mi_malloc(n);
    if (!p) {
        throw std::bad_alloc{};
    }
    SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new[](std::size_t n) {
    auto* p = mi_malloc(n);
    if (!p) {
        throw std::bad_alloc{};
    }
    SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new(std::size_t n, std::nothrow_t const&) noexcept {
    auto* p = mi_malloc(n);
    if (p) {
        SPONGE_PROFILE_ALLOC(p, n);
    }
    return p;
}
void* operator new[](std::size_t n, std::nothrow_t const&) noexcept {
    auto* p = mi_malloc(n);
    if (p) {
        SPONGE_PROFILE_ALLOC(p, n);
    }
    return p;
}
void* operator new(std::size_t n, std::align_val_t al) {
    auto* p = mi_malloc_aligned(n, static_cast<std::size_t>(al));
    if (!p) {
        throw std::bad_alloc{};
    }
    SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new[](std::size_t n, std::align_val_t al) {
    auto* p = mi_malloc_aligned(n, static_cast<std::size_t>(al));
    if (!p) {
        throw std::bad_alloc{};
    }
    SPONGE_PROFILE_ALLOC(p, n);
    return p;
}
void* operator new(std::size_t n, std::align_val_t al,
                   std::nothrow_t const&) noexcept {
    auto* p = mi_malloc_aligned(n, static_cast<std::size_t>(al));
    if (p) {
        SPONGE_PROFILE_ALLOC(p, n);
    }
    return p;
}
void* operator new[](std::size_t n, std::align_val_t al,
                     std::nothrow_t const&) noexcept {
    auto* p = mi_malloc_aligned(n, static_cast<std::size_t>(al));
    if (p) {
        SPONGE_PROFILE_ALLOC(p, n);
    }
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
