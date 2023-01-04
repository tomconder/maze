project(glad C CXX)

file(GLOB HEADER_FILES LIST_DIRECTORIES false
        src/include/*.h)

file(GLOB SOURCE_FILES LIST_DIRECTORIES false
        src/*.c)

#add_library(glad ${HEADER_FILES} ${SOURCE_FILES})
add_library(glad EXCLUDE_FROM_ALL src/gl.c)

target_include_directories(glad PUBLIC include)

install(TARGETS glad
        BUNDLE DESTINATION bin
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)

install(FILES ${HEADER_FILES} DESTINATION glad)
