#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace assetconv {

// Packs several small images into one KTX2 sprite sheet. The rect table goes
// in the container's key/value data, so an atlas is still one file.
//
// Only for UI art: CLAMP_TO_EDGE, no tiling, drawn at or near native size.
// Model textures cannot be atlased, because they repeat.
struct AtlasEntry {
    std::string name;  // sprite name the engine looks up
    std::string path;  // source image on disk
};

// Returns the KTX2 file, or an empty vector after logging on failure.
std::vector<uint8_t> packAtlas(const std::vector<AtlasEntry>& entries);

}  // namespace assetconv
