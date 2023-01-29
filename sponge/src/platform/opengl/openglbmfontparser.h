#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct bmchar {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    int32_t xoffset;
    int32_t yoffset;
    int32_t xadvance;
    uint32_t page;
};

class OpenGLBMFontParser {
   public:
    void parse(const std::string& source) const;
};
