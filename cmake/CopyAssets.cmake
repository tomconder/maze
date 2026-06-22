# copy_assets.cmake — copy source assets to destination, excluding Slang
# sources, then overlay compiled GLSL shaders.
# Required variables: SRC_DIR, DST_DIR, SRC_GLSL, EXCLUDE_SUBDIR
file(COPY "${SRC_DIR}/"
     DESTINATION "${DST_DIR}"
     FILES_MATCHING
     PATTERN "*"
     PATTERN "${EXCLUDE_SUBDIR}" EXCLUDE
     PATTERN "${EXCLUDE_SUBDIR}/*" EXCLUDE)

file(COPY "${SRC_GLSL}/"
     DESTINATION "${DST_DIR}/shaders/glsl")
