set(ELPP_NO_DEFAULT_LOG_FILE TRUE CACHE INTERNAL "override easyloggingpp flags" FORCE)
set(build_static_lib ON CACHE INTERNAL "Build easyloggingpp as a static library" FORCE)

add_subdirectory(easyloggingpp)

add_subdirectory(glad)

add_subdirectory(stb_image)
