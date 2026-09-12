# Asset converter. Links sponge for the importers, the KTX2 codec and the
# baked format writer, so the converter and the engine can never disagree
# about the layout.
file(
        GLOB SOURCE_FILES
        CONFIGURE_DEPENDS
        LIST_DIRECTORIES false
        src/*.cpp)

file(
        GLOB HEADER_FILES
        CONFIGURE_DEPENDS
        LIST_DIRECTORIES false
        src/*.hpp)

add_executable(assetconv ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(assetconv PRIVATE src)

target_link_libraries(assetconv PRIVATE sponge::sponge)

set_target_properties(assetconv PROPERTIES CXX_STANDARD 23)

# Host tool: never installed with the game.
