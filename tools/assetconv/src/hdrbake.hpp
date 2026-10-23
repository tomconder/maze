#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace assetconv {

// Reads a Radiance .hdr (RGBE) image and returns it as a KTX2 file of
// half-float RGBA with one level. Values past the half-float range clamp to
// its maximum. Prints the reason to stderr and returns an empty vector on
// failure.
std::vector<uint8_t> bakeEnvironment(const std::string& source);

}  // namespace assetconv
