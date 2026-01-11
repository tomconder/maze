# Maze

A game engine featuring a nice walk through a maze.

> For more information please check out the [full documentation](https://tomconder.github.io/maze/)

![work in progress](docs/docs/static/img/workinprogress.png "work in progress")

## Features

* Platform portability: OpenGL and C++ for Windows and macOS
* Physically based rendering (PBR)
* Cook-Torrance microfacet specular BRDF
* Point lights and directional light
* PCF shadows
* Reinhard tone mapping
* Multi-channel signed distance field (MSDF) text rendering
* Performance profiling with Tracy

## Project Organization

```
assets/          # game assets
docs/            # documentation
game/            # source files for maze
sponge/          # source files for sponge game engine
tools/           # tools and utility scripts
```

## Installing

Clone this repository.

```
git clone https://github.com/tomconder/maze.git
```

### Install vcpkg

> \[!IMPORTANT]
> The `VCPKG_ROOT` environment variable must be set so that the CMake can find vcpkg to install dependencies for the build.

[Install vcpkg](https://github.com/microsoft/vcpkg#getting-started), a dependency and package manager for C++.

By far the quickest way to install vcpkg is to clone it into this project.

```
cd maze
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
cd ..
setx VCPKG_ROOT vcpkg
```

If you installed vcpkg elsewhere, add an environment variable called `VCPKG_ROOT` that contains the location where you
installed vcpkg.

```
setx VCPKG_ROOT <path to vcpkg>
```

### Install CMake

[Install CMake](https://cmake.org/install/), a cross-platform build system.

For example, use [Chocolatey](https://chocolatey.org/install) to install CMake.

```
choco install cmake
```

## Building

Use a configuration preset to compile `maze`. Possible values are:

| Preset | Description                          |
| ------ |--------------------------------------|
| `ci-linux-debug` | Linux debug build                    |
| `ci-linux-release` | Linux release build                  |
| `ci-osx-debug` | MacOS debug build                    |
| `ci-osx-release` | MacOS release build                  |
| `ci-windows-debug` | Windows debug build (uses sccache)   |
| `ci-windows-release` | Windows release build (uses sccache) |
| `windows-msvc-debug` | Windows MSVC debug build             |
| `windows-msvc-release` | Windows MSVC release build           |

For example, to use the preset to on Windows:

```
cmake.exe -B build --preset windows-msvc-release
cmake.exe --build build --target game --config Release
```

## Running

On Windows, you can find the maze executable will be found in the build directory.

```
build\maze\Release\maze.exe
```

Or, for MacOS, the app bundle will be found in the build directory.

```
build/maze/Release/maze.app
```
