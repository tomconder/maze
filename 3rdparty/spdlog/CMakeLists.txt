add_library(spdlog INTERFACE EXCLUDE_FROM_ALL)

target_include_directories(spdlog INTERFACE include)

install(FILES ${HEADER_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
