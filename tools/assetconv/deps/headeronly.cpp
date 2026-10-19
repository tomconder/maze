// Implementations of the header-only libraries.
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

// clusterlod.h comes from meshoptimizer's demo folder at the tag of the
// vcpkg port, and calls the API of that version. Update both together.
#include <meshoptimizer.h>
static_assert(MESHOPTIMIZER_VERSION == 1030,
              "copy deps/clusterlod/clusterlod.h from the same meshoptimizer tag");
#define CLUSTERLOD_IMPLEMENTATION
#include <clusterlod.h>
