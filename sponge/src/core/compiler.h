#pragma once

#ifndef DISABLE_ALL_VC_WARNINGS

#if defined(_MSC_VER)

#define DISABLE_ALL_VC_WARNINGS()                \
    __pragma(warning(push, 0)) __pragma(warning( \
        disable : 4244 4265 4267 4350 4472 4509 4548 4623 4710 4985 6320 4755 4625 4626 4702))

#else

#define DISABLE_ALL_VC_WARNINGS()

#endif
#endif

#ifndef RESTORE_ALL_VC_WARNINGS

#if defined(_MSC_VER)

#define RESTORE_ALL_VC_WARNINGS() __pragma(warning(pop))

#else

#define RESTORE_ALL_VC_WARNINGS()

#endif
#endif
