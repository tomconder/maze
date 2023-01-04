#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

#define ELPP_NO_DEFAULT_LOG_FILE
#include "easylogging++.h"
#include "maze.h"

INITIALIZE_EASYLOGGINGPP

std::unique_ptr<Maze> maze;

// loop iteration is broken out like this for emscripten
bool iterateLoop()
{
    return maze->iterateLoop();
}

extern "C" int main(int argc, char *args[])
{
    el::Configurations logConf;
    logConf.setToDefault();
    logConf.setGlobally(el::ConfigurationType::Filename, "log.txt");
    logConf.setGlobally(el::ConfigurationType::Format,
                        "%level %datetime{%Y-%M-%d %H:%m:%s,%g} [%logger] %fbase - %msg");
    logConf.setGlobally(el::ConfigurationType::SubsecondPrecision, "3");
    logConf.setGlobally(el::ConfigurationType::LogFlushThreshold, "100");
    // Max Log File Size = 10 Mb
    logConf.setGlobally(el::ConfigurationType::MaxLogFileSize, "10485760");

    el::Loggers::reconfigureAllLoggers(logConf);

    maze = std::make_unique<Maze>();

    if (maze->construct(globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT) == 0) {
        return 1;
    }

    if (maze->start() == 0) {
        return 1;
    }

#ifdef EMSCRIPTEN
    emscripten_set_main_loop((em_callback_func)iterateLoop, 60, true);
#else
    bool quit = false;
    while (!quit) {
        quit = iterateLoop();
    }
#endif

    return 0;
}
