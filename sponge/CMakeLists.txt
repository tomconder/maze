# Sponge Engine
include(GNUInstallDirs)

file(
        GLOB HEADER_FILES
        LIST_DIRECTORIES false
        src/*.hpp
        src/core/*.hpp
        src/debug/*.hpp
        src/event/*.hpp
        src/input/*.hpp
        src/layer/*.hpp
        src/logging/*.hpp
        src/renderer/*.hpp
        src/scene/*.hpp)

file(
        GLOB SOURCE_FILES
        LIST_DIRECTORIES false
        src/core/*.cpp
        src/event/*.cpp
        src/input/*.cpp
        src/layer/*.cpp
        src/logging/*.cpp
        src/renderer/*.cpp
        src/scene/*.cpp)

# OpenGL
message(STATUS "Including OpenGL platform files")
file(
        GLOB OPENGL_HEADERS
        LIST_DIRECTORIES false
        src/platform/opengl/debug/*.hpp
        src/platform/opengl/renderer/*.hpp
        src/platform/opengl/scene/*.hpp)
list(APPEND HEADER_FILES ${OPENGL_HEADERS})
file(
        GLOB OPENGL_SOURCES
        LIST_DIRECTORIES false
        deps/glad2/src/gl.c
        src/platform/opengl/renderer/*.cpp
        src/platform/opengl/scene/*.cpp)
list(APPEND SOURCE_FILES ${OPENGL_SOURCES})

# glfw
message(STATUS "Including glfw platform files")
file(
        GLOB GLFW_HEADERS
        LIST_DIRECTORIES false
        src/platform/glfw/core/*.hpp
        src/platform/glfw/logging/*.hpp
)
list(APPEND HEADER_FILES ${GLFW_HEADERS})
file(
        GLOB GLFW_SOURCES
        LIST_DIRECTORIES false
        src/platform/glfw/core/*.cpp
        src/platform/glfw/input/*.cpp
        src/platform/glfw/logging/*.cpp
)
list(APPEND SOURCE_FILES ${GLFW_SOURCES})

if (ENABLE_IMGUI)
    message(STATUS "Including imgui platform files")
    file(
            GLOB IMGUI_HEADERS
            LIST_DIRECTORIES false
            src/imgui/*.hpp
            src/platform/glfw/imgui/*.hpp
    )
    list(APPEND HEADER_FILES ${IMGUI_HEADERS})
    file(
            GLOB IMGUI_SOURCES
            LIST_DIRECTORIES false
            src/imgui/*.cpp
            src/platform/glfw/imgui/*.cpp
    )
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

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(meshoptimizer CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)

find_package(OpenGL REQUIRED)
find_package(Stb REQUIRED)

if (ENABLE_IMGUI)
    find_package(imgui CONFIG REQUIRED)
endif ()

if (ENABLE_PROFILING)
    find_package(Tracy CONFIG REQUIRED)
endif ()

add_library(sponge ${HEADER_FILES} ${SOURCE_FILES})
add_library(sponge::sponge ALIAS sponge)

if (MSVC)
    # hardening
    target_compile_options(sponge PUBLIC $<$<NOT:$<CONFIG:DEBUG>>:/DYNAMICBASE /guard:cf>)
    target_link_options(sponge PUBLIC "/NXCOMPAT /CETCOMPAT")

    # properly report what c++ version is being used
    target_compile_options(sponge INTERFACE /Zc:__cplusplus)

    # fast math and SIMD support
    # https://learn.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior?view=msvc-170#fast
    target_compile_options(sponge INTERFACE $<$<CONFIG:RELEASE>:/fp:fast>)

    # do not omit frame pointers for crash dumps
    target_compile_options(sponge INTERFACE /Oy-)

    # enable exception handling
    target_compile_options(sponge INTERFACE /EHsc)

    # enable warnings and warnings as errors
    target_compile_options(sponge INTERFACE /W4 /WX)

    # disable RTTI
    target_compile_options(sponge INTERFACE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
else ()
    # see the implications here: https://stackoverflow.com/q/45685487
    target_compile_options(sponge INTERFACE $<$<CONFIG:RELEASE>:-Ofast>)

    # do not omit frame pointers for crash dumps
    target_compile_options(sponge INTERFACE -fno-omit-frame-pointer)

    # enable exception handling
    target_compile_options(sponge INTERFACE -fexceptions)

    # enable warnings and warnings as errors
    target_compile_options(sponge INTERFACE -Wall -Werror)

    # disable RTTI
    target_compile_options(sponge INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)

    # enable separate sections per function and data item
    target_compile_options(sponge INTERFACE $<$<CONFIG:RELEASE>:-ffunction-sections>)
    target_compile_options(sponge INTERFACE $<$<CONFIG:RELEASE>:-fdata-sections>)

    # discard unused sections
    target_link_options(sponge PUBLIC $<$<CONFIG:RELEASE>:$<$<CXX_COMPILER_ID:Clang,GNU>:-Wl,--gc-sections>>)
    target_link_options(sponge PUBLIC $<$<CONFIG:RELEASE>:$<$<CXX_COMPILER_ID:AppleClang>:-Wl,-dead_strip>>)
endif ()

set_target_properties(sponge PROPERTIES CXX_STANDARD 23)

target_include_directories(sponge PUBLIC src)
target_include_directories(sponge PUBLIC deps/glad2/include)
target_include_directories(sponge SYSTEM PRIVATE ${Stb_INCLUDE_DIR})

target_link_libraries(sponge PRIVATE glfw)
target_link_libraries(sponge PRIVATE glm::glm)
target_link_libraries(sponge PRIVATE meshoptimizer::meshoptimizer)
target_link_libraries(sponge PRIVATE OpenGL::GL)
target_link_libraries(sponge PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(sponge PRIVATE tinyobjloader::tinyobjloader)
if (ENABLE_IMGUI)
    target_link_libraries(sponge PRIVATE imgui::imgui)
endif ()
if (ENABLE_PROFILING)
    target_link_libraries(sponge PRIVATE Tracy::TracyClient)
endif ()
