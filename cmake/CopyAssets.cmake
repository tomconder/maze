# copy_assets.cmake — copy source assets to destination, excluding Slang
# sources, bake inputs and license files, then overlay baked assets.
#
# EXCLUDE_REGEX is DEPLOY_EXCLUDE_REGEX from ConvertAssets.cmake, which lists
# what it drops and why.
#
# assets/models is excluded wholesale instead, because it holds nothing but
# source art. Every model the runtime loads comes from the bake, so a file
# left there is either an input or unused - shipping it either way is waste.
#
# Required variables: SRC_DIR, DST_DIR, SRC_BAKED, EXCLUDE_SUBDIR,
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

# Baked assets last: they overlay the raw sources they were built from.
file(COPY "${SRC_BAKED}/"
     DESTINATION "${DST_DIR}")
