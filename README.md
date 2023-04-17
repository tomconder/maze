# Maze

A nice walk through a maze.

> For more information please check out the [full documentation](https://tomconder.github.io/maze/)

## Project Organization

    3rdparty/        # third party dependencies
    build-scripts/   # build utility scripts
    cmake/           # cmake specific files
    docs/            # documentation
    game/            # source files for maze
    sponge/          # source files for sponge game engine


## Installing

Clone this repository.

```
git clone https://github.com/tomconder/maze.git
```

[Install vcpkg](https://github.com/microsoft/vcpkg#getting-started), a dependency and package manager for C++.

Set `VCPKG_ROOT` to the location where you installed vcpkg

```
export VCPKG_ROOT <path to vcpkg>
```

[Install CMake](https://cmake.org/install/), a cross-platform build system.

Now you can use a preset to compile `maze`. Possible values are: x64-debug, x64-release, emscripten, osx-debug, osx-release, linux-debug, linux-release
```
cmake --preset x64-debug
cmake --build out/build/x64-debug --target game
```
