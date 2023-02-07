add_library(glad EXCLUDE_FROM_ALL src/glad.c)

target_include_directories(glad PUBLIC include)
