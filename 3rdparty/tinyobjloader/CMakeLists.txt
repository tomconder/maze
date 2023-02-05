add_library(tinyobjloader INTERFACE EXCLUDE_FROM_ALL)

target_include_directories(tinyobjloader INTERFACE include)

install(FILES ${HEADER_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
