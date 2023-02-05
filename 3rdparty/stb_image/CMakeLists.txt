add_library(stb_image INTERFACE EXCLUDE_FROM_ALL)

target_include_directories(stb_image INTERFACE include)

install(FILES ${HEADER_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
