# copy_assets.cmake — copy source assets to destination, excluding Slang
# sources and anything the manifest bakes, then overlay compiled GLSL shaders
# and baked assets.
#
# EXCLUDE_REGEX drops the manifest's own inputs: a source that was baked must
# not ship next to the file it was baked into. It names individual files
# rather than directories so that licence files sitting beside the art they
# cover still deploy.
#
# assets/models is excluded wholesale instead, because it holds nothing but
# source art. Every model the runtime loads comes from the bake, so a file
# left there is either an input or unused - shipping it either way is waste.
#
# Required variables: SRC_DIR, DST_DIR, SRC_GLSL, SRC_BAKED, EXCLUDE_SUBDIR,
# EXCLUDE_REGEX
file(COPY "${SRC_DIR}/"
     DESTINATION "${DST_DIR}"
     FILES_MATCHING
     PATTERN "*"
     REGEX "${EXCLUDE_REGEX}" EXCLUDE
     PATTERN ".clang-format" EXCLUDE
     PATTERN "models" EXCLUDE
     PATTERN "${EXCLUDE_SUBDIR}" EXCLUDE
     PATTERN "${EXCLUDE_SUBDIR}/*" EXCLUDE)

file(COPY "${SRC_GLSL}/"
     DESTINATION "${DST_DIR}/shaders/glsl")

# Baked assets last: they overlay the raw sources they were built from.
file(COPY "${SRC_BAKED}/"
     DESTINATION "${DST_DIR}")
