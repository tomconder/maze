cmake_minimum_required(VERSION 2.8.7)

project(easyloggingpp CXX)

macro(require_cpp11)
  if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 3.0)
    # CMake 3.1 has built-in CXX standard checks.
    message("-- Setting C++11")
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED on)
  else ()
    if (CMAKE_CXX_COMPILER_ID MATCHES "GCC")
      message("-- GNU CXX (-std=c++11)")
      list(APPEND CMAKE_CXX_FLAGS "-std=c++11")
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      message("-- CLang CXX (-std=c++11)")
      list(APPEND CMAKE_CXX_FLAGS "-std=c++11")
    else ()
      message("-- Easylogging++ requires C++11. Your compiler does not support it.")
    endif ()
  endif ()
endmacro()

option(lib_utc_datetime "Build library with UTC date/time logging" OFF)
option(build_static_lib "Build easyloggingpp as a static library" ON)
option(enable_default_logfile "Enable default log file" OFF)

set(ELPP_MAJOR_VERSION "9")
set(ELPP_MINOR_VERSION "96")
set(ELPP_PATCH_VERSION "7")
set(ELPP_VERSION_STRING "${ELPP_MAJOR_VERSION}.${ELPP_MINOR_VERSION}.${ELPP_PATCH_VERSION}")
set(ELPP_NO_DEFAULT_LOG_FILE True)

set(ELPP_INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH "The directory the headers are installed in")
set(ELPP_PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/pkgconfig" CACHE PATH "Installation directory for pkgconfig (.pc) files")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

install(FILES
        src/easylogging++.h
        src/easylogging++.cc
        DESTINATION "${ELPP_INCLUDE_INSTALL_DIR}"
        COMPONENT dev)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/easyloggingpp.pc.cmakein
        ${CMAKE_CURRENT_BINARY_DIR}/easyloggingpp.pc @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/easyloggingpp.pc DESTINATION "${ELPP_PKGCONFIG_INSTALL_DIR}")

include(CheckIncludeFileCXX)
check_include_file_cxx("execinfo.h" HAVE_EXECINFO)
if (HAVE_EXECINFO)
  add_definitions(-DHAVE_EXECINFO)
endif ()

if (lib_utc_datetime)
  add_definitions(-DELPP_UTC_DATETIME)
endif ()

require_cpp11()
add_library(easyloggingpp EXCLUDE_FROM_ALL src/easylogging++.cc src/easylogging++.h)

target_include_directories(easyloggingpp INTERFACE src)

install(TARGETS
        easyloggingpp
        BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
