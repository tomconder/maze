add_library(glad EXCLUDE_FROM_ALL src/glad.c)

target_include_directories(glad PUBLIC include include)

install(FILES ${HEADER_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
