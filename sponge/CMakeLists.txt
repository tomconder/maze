#Sponge Engine
include(GNUInstallDirs)

file(
        GLOB HEADER_FILES
        CONFIGURE_DEPENDS
        LIST_DIRECTORIES false
        src/*.hpp
        src/core/*.hpp
        src/debug/*.hpp
        src/event/*.hpp
        src/input/*.hpp
        src/layer/*.hpp
        src/logging/*.hpp
        src/scene/*.hpp
        src/thread/*.hpp)

file(
        GLOB SOURCE_FILES
        CONFIGURE_DEPENDS
        LIST_DIRECTORIES false
        src/core/*.cpp src/event/*.cpp src/input/*.cpp src/layer/*.cpp
        src/logging/*.cpp src/scene/*.cpp src/thread/*.cpp)

# OpenGL
message(STATUS "Including OpenGL platform files")
file(
        GLOB OPENGL_HEADERS
        CONFIGURE_DEPENDS
        LIST_DIRECTORIES false
        src/platform/opengl/debug/*.hpp src/platform/opengl/renderer/*.hpp
        src/platform/opengl/scene/*.hpp)
list(APPEND HEADER_FILES ${OPENGL_HEADERS})
file(
        GLOB OPENGL_SOURCES
        CONFIGURE_DEPENDS
        LIST_DIRECTORIES false
        deps/glad2/src/gl.c src/platform/opengl/debug/*.cpp
        src/platform/opengl/renderer/*.cpp src/platform/opengl/scene/*.cpp)
list(APPEND SOURCE_FILES ${OPENGL_SOURCES})

# glfw
message(STATUS "Including glfw platform files")
file(
        GLOB GLFW_HEADERS
        LIST_DIRECTORIES false
        src/platform/glfw/core/*.hpp src/platform/glfw/logging/*.hpp)
list(APPEND HEADER_FILES ${GLFW_HEADERS})
file(
        GLOB GLFW_SOURCES
        LIST_DIRECTORIES false
        src/platform/glfw/core/*.cpp src/platform/glfw/input/*.cpp
        src/platform/glfw/logging/*.cpp)
list(APPEND SOURCE_FILES ${GLFW_SOURCES})

# audio
message(STATUS "Including audio platform files")
file(
        GLOB AUDIO_HEADERS
        LIST_DIRECTORIES false
        src/platform/audio/*.hpp)
list(APPEND HEADER_FILES ${AUDIO_HEADERS})
file(
        GLOB AUDIO_SOURCES
        LIST_DIRECTORIES false
        src/platform/audio/*.cpp)
list(APPEND SOURCE_FILES ${AUDIO_SOURCES})

if (ENABLE_IMGUI)
    message(STATUS "Including imgui platform files")
    file(
            GLOB IMGUI_HEADERS
            LIST_DIRECTORIES false
            src/imgui/*.hpp src/platform/glfw/imgui/*.hpp)
    list(APPEND HEADER_FILES ${IMGUI_HEADERS})
    file(
            GLOB IMGUI_SOURCES
            LIST_DIRECTORIES false
            src/imgui/*.cpp src/platform/glfw/imgui/*.cpp)
    list(APPEND SOURCE_FILES ${IMGUI_SOURCES})
endif ()

if (APPLE)
    message(STATUS "Including OSX platform files")
    file(GLOB OSX_HEADERS src/platform/osx/core/*.hpp)
    list(APPEND HEADER_FILES ${OSX_HEADERS})
    file(GLOB OSX_SOURCES src/platform/osx/core/*.cpp)
    list(APPEND SOURCE_FILES ${OSX_SOURCES})
endif ()

if (MSVC)
    message(STATUS "Including Windows platform files")
    file(GLOB WIN_HEADERS src/platform/windows/core/*.hpp)
    list(APPEND HEADER_FILES ${WIN_HEADERS})
    file(GLOB WIN_SOURCES src/platform/windows/core/*.cpp)
    list(APPEND SOURCE_FILES ${WIN_SOURCES})
endif ()

if (UNIX AND NOT APPLE)
    message(STATUS "Including Linux platform files")
    file(GLOB LINUX_HEADERS src/platform/linux/core/*.hpp)
    list(APPEND HEADER_FILES ${LINUX_HEADERS})
    file(GLOB LINUX_SOURCES src/platform/linux/core/*.cpp)
    list(APPEND SOURCE_FILES ${LINUX_SOURCES})
endif ()

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

find_package(OpenGL REQUIRED)

if (ENABLE_IMGUI)
    find_package(imgui CONFIG REQUIRED)
endif ()

if (ENABLE_PROFILING)
    find_package(Tracy CONFIG REQUIRED)
endif ()

# Build options shared by the engine and the format library, and so by
# assetconv through the latter. Warnings are a separate target so vendored
# code can take the options without /W4: a per-source /W0 cannot remove a
# target's /W4, and MSVC warns D9025 when it overrides it.
add_library(sponge_options INTERFACE)
add_library(sponge_warnings INTERFACE)
add_library(sponge::options ALIAS sponge_options)

# miniaudio, vendored: no warnings.
add_library(sponge_miniaudio OBJECT deps/miniaudio/miniaudio.cpp)
set_target_properties(sponge_miniaudio PROPERTIES CXX_STANDARD 23)
target_link_libraries(sponge_miniaudio PRIVATE sponge_options)
target_compile_options(sponge_miniaudio PRIVATE
        $<IF:$<CXX_COMPILER_ID:MSVC>,/W0,-w>)

# Baked asset formats: readers for the engine, writers for assetconv. No
# logging and no engine headers, so an engine edit does not relink assetconv
# and rebake every asset.
find_package(fmt CONFIG REQUIRED)
file(
        GLOB FORMAT_FILES
        CONFIGURE_DEPENDS
        LIST_DIRECTORIES false
        format/scene/src/*.cpp format/scene/src/*.hpp)
add_library(sponge_format STATIC ${FORMAT_FILES})
add_library(sponge::format ALIAS sponge_format)
set_target_properties(sponge_format PROPERTIES CXX_STANDARD 23)
target_include_directories(sponge_format PUBLIC format/scene/src)
target_link_libraries(sponge_format PUBLIC sponge_options sponge_warnings
        glm::glm-header-only)
target_link_libraries(sponge_format PRIVATE fmt::fmt)

add_library(sponge ${HEADER_FILES} ${SOURCE_FILES})
add_library(sponge::sponge ALIAS sponge)
target_link_libraries(sponge PUBLIC sponge_format)
target_link_libraries(sponge PRIVATE sponge_miniaudio)

if (MSVC)
    # hardening
    target_compile_options(sponge_options INTERFACE $<$<NOT:$<CONFIG:DEBUG>>:/DYNAMICBASE
            /guard:cf>)

    # properly report what c++ version is being used
    target_compile_options(sponge_options INTERFACE /Zc:__cplusplus)

    # fast math and SIMD support
    # https://learn.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior?view=msvc-170#fast
    target_compile_options(sponge_options INTERFACE $<$<CONFIG:RELEASE>:/fp:fast>)

    # do not omit frame pointers for crash dumps
    target_compile_options(sponge_options INTERFACE /Oy-)

    # enable exception handling
    target_compile_options(sponge_options INTERFACE /EHsc)

    # enable warnings as errors
    target_compile_options(sponge_warnings INTERFACE /W4 /WX)

    # disable RTTI
    target_compile_options(sponge_options INTERFACE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
else ()
    # see the implications here: https://stackoverflow.com/q/45685487
    target_compile_options(sponge_options INTERFACE $<$<CONFIG:RELEASE>:-ffast-math>)

    # do not omit frame pointers for crash dumps
    target_compile_options(sponge_options INTERFACE -fno-omit-frame-pointer)

    # enable exception handling
    target_compile_options(sponge_options INTERFACE -fexceptions)

    # enable warnings as errors
    target_compile_options(sponge_warnings INTERFACE -Wall -Werror)

    # disable RTTI
    target_compile_options(sponge_options INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)

    # enable separate sections per function and data item
    target_compile_options(sponge_options
            INTERFACE $<$<CONFIG:RELEASE>:-ffunction-sections>)
    target_compile_options(sponge_options INTERFACE $<$<CONFIG:RELEASE>:-fdata-sections>)

    # discard unused sections
    target_link_options(
            sponge_options INTERFACE
            $<$<CONFIG:RELEASE>:$<$<CXX_COMPILER_ID:Clang,GNU>:-Wl,--gc-sections>>)
    target_link_options(
            sponge_options INTERFACE
            $<$<CONFIG:RELEASE>:$<$<CXX_COMPILER_ID:AppleClang>:-Wl,-dead_strip>>)
endif ()

set_target_properties(sponge PROPERTIES CXX_STANDARD 23)

target_compile_definitions(sponge PUBLIC GLFW_INCLUDE_NONE)
target_compile_definitions(sponge PUBLIC
        SPDLOG_ACTIVE_LEVEL=$<IF:$<CONFIG:Debug>,SPDLOG_LEVEL_TRACE,SPDLOG_LEVEL_ERROR>)

target_include_directories(sponge PUBLIC src)
target_include_directories(sponge PUBLIC deps/glad2/include)
target_include_directories(sponge PRIVATE deps/miniaudio)

target_link_libraries(sponge PUBLIC glfw)
target_link_libraries(sponge PUBLIC glm::glm-header-only)
target_link_libraries(sponge PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(sponge PRIVATE OpenGL::GL)
target_link_libraries(sponge PUBLIC spdlog::spdlog spdlog::spdlog_header_only)
if (APPLE)
    target_link_libraries(sponge PRIVATE "-framework AudioToolbox"
            "-framework CoreAudio" "-framework CoreFoundation")
elseif (UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(sponge PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
endif ()
if (ENABLE_IMGUI)
    target_link_libraries(sponge PUBLIC imgui::imgui)
endif ()
if (ENABLE_PROFILING)
    target_link_libraries(sponge PUBLIC Tracy::TracyClient)
endif ()
