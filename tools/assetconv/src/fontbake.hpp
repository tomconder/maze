#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace assetconv {

// Rasterizes a font at each pixel size into an LCD coverage atlas and bakes
// the shaping tables beside it (see font.hpp). Returns the KTX2 file, or an
// empty vector after printing on failure, including when the tables do not
// reproduce HarfBuzz.
std::vector<uint8_t> bakeFont(const std::string&           path,
                              const std::vector<uint32_t>& sizes);

}  // namespace assetconv
