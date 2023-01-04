#ifndef INCLUDE_GLOBALS_H
#define INCLUDE_GLOBALS_H

#define UNUSED(x) (void)(x)

namespace globals
{
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

enum class Retcode
{
    FAIL = 0,
    OK = 1
};
} // namespace globals

#endif // INCLUDE_GLOBALS_H
