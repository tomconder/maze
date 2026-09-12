# copy_assets.cmake — copy source assets to destination, excluding Slang
# sources, then overlay compiled GLSL shaders and baked assets.
# Required variables: SRC_DIR, DST_DIR, SRC_GLSL, SRC_BAKED, EXCLUDE_SUBDIR
file(COPY "${SRC_DIR}/"
     DESTINATION "${DST_DIR}"
     FILES_MATCHING
     PATTERN "*"
     PATTERN "${EXCLUDE_SUBDIR}" EXCLUDE
     PATTERN "${EXCLUDE_SUBDIR}/*" EXCLUDE)

file(COPY "${SRC_GLSL}/"
     DESTINATION "${DST_DIR}/shaders/glsl")

# Baked assets last: they overlay the raw sources they were built from.
file(COPY "${SRC_BAKED}/"
     DESTINATION "${DST_DIR}")
