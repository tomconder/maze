# Sponge Engine

include(GNUInstallDirs)

file(GLOB HEADER_FILES LIST_DIRECTORIES false
        src/*.h
        src/core/*.h
        src/platform/opengl/*.h
        src/renderer/*.h)

file(GLOB SOURCE_FILES LIST_DIRECTORIES false
        src/*.cpp
        src/core/*.cpp
        src/platform/opengl/*.cpp
        src/renderer/*.cpp)

file(GLOB SHADERS LIST_DIRECTORIES false
        shaders/*.frag
        shaders/*.vert)

# 01/2022 - conan does not pre-build cygwin gcc so you must target the visual studio toolchain

if (NOT EXISTS "${CMAKE_BINARY_DIR}/sponge/conan.cmake")
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake"
          "${CMAKE_BINARY_DIR}/sponge/conan.cmake"
          TLS_VERIFY ON)
endif ()

include(${CMAKE_BINARY_DIR}/sponge/conan.cmake)

conan_cmake_configure(REQUIRES
        assimp/5.2.2
        easyloggingpp/9.97.0
        freetype/2.12.1
        glm/0.9.9.8
        sdl/2.26.1
        zlib/1.2.13
        GENERATORS cmake_find_package
        OPTIONS
        assimp:double_precision=True
        assimp:with_3d=False
        assimp:with_3ds=False
        assimp:with_3ds_exporter=False
        assimp:with_3mf=False
        assimp:with_3mf_exporter=False
        assimp:with_ac=False
        assimp:with_amf=False
        assimp:with_ase=False
        assimp:with_assbin=False
        assimp:with_assbin_exporter=False
        assimp:with_assjson_exporter=False
        assimp:with_assxml_exporter=False
        assimp:with_b3d=False
        assimp:with_blend=False
        assimp:with_bvh=False
        assimp:with_cob=False
        assimp:with_collada=False
        assimp:with_collada_exporter=False
        assimp:with_csm=False
        assimp:with_dxf=False
        assimp:with_fbx=False
        assimp:with_fbx_exporter=False
        assimp:with_gltf=False
        assimp:with_gltf_exporter=False
        assimp:with_hmp=False
        assimp:with_ifc=False
        assimp:with_iqm=False
        assimp:with_irr=False
        assimp:with_irrmesh=False
        assimp:with_lwo=False
        assimp:with_lws=False
        assimp:with_m3d=False
        assimp:with_m3d_exporter=False
        assimp:with_md2=False
        assimp:with_md3=False
        assimp:with_md5=False
        assimp:with_mdc=False
        assimp:with_mdl=False
        assimp:with_mmd=False
        assimp:with_ms3d=False
        assimp:with_ndo=False
        assimp:with_nff=False
        assimp:with_obj=True
        assimp:with_obj_exporter=False
        assimp:with_off=False
        assimp:with_ogre=False
        assimp:with_opengex=False
        assimp:with_opengex_exporter=False
        assimp:with_pbrt_exporter=False
        assimp:with_ply=False
        assimp:with_ply_exporter=False
        assimp:with_q3d=False
        assimp:with_raw=False
        assimp:with_sib=False
        assimp:with_smd=False
        assimp:with_step=False
        assimp:with_step_exporter=False
        assimp:with_stl=False
        assimp:with_stl_exporter=False
        assimp:with_terragen=False
        assimp:with_x3d=False
        assimp:with_x3d_exporter=False
        assimp:with_x=False
        assimp:with_x_exporter=False
        assimp:with_xgl=False
        easyloggingpp:enable_default_logfile=False)

conan_cmake_autodetect(settings)

conan_cmake_install(PATH_OR_REFERENCE .
        CMAKE_TARGETS
        OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
        BUILD missing
        REMOTE conancenter
        SETTINGS ${settings})

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})

find_package(Freetype REQUIRED)
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(assimp REQUIRED)
find_package(easyloggingpp REQUIRED)
find_package(glm REQUIRED)

# for debugging
# include(CMakePrintHelpers)
# cmake_print_variables(SDL2_INCLUDE_DIR)
# cmake_print_variables(SDL2_LIBRARY)

add_library(sponge ${HEADER_FILES} ${SOURCE_FILES})

if (MSVC)
  target_compile_options(sponge PUBLIC "/Zi")
  target_link_options(sponge PUBLIC "/INCREMENTAL")
endif ()

target_include_directories(sponge PUBLIC src)

target_link_libraries(sponge PRIVATE Freetype::Freetype)
target_link_libraries(sponge PRIVATE OpenGL::GL)
target_link_libraries(sponge PRIVATE SDL2::SDL2)
target_link_libraries(sponge PRIVATE assimp::assimp)
target_link_libraries(sponge PRIVATE easyloggingpp::easyloggingpp)
target_link_libraries(sponge PRIVATE glad)
target_link_libraries(sponge PRIVATE glm::glm)
target_link_libraries(sponge PRIVATE stb)

install(TARGETS sponge
        BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(FILES ${HEADER_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_custom_command(
        TARGET sponge POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders ${CMAKE_SOURCE_DIR}/build/assets/shaders
        COMMENT "Copying shaders" VERBATIM
        DEPENDS ${SHADERS})
