#include "openglbmfontparser.h"

#include <fstream>
#include <sstream>
#include <vector>

#include "core/log.h"
#include "spdlog/fmt/bundled/ranges.h"

std::unordered_map<uint32_t, bmchar> fontChars;

void OpenGLBMFontParser::parse(const std::string& path) const {
    assert(!path.empty());

    SPONGE_CORE_DEBUG("loadFromBMFile: {}", path);

    std::ifstream stream(path, std::ios::in | std::ios::binary);
    assert(stream.good());

    auto nextValue = [](std::stringstream& sstream) {
        std::string s;
        sstream >> s;
        if (size_t pos = s.find_last_of('='); pos != std::string::npos) {
            return std::stoi(s.substr(pos + 1));
        }
        return 0;
    };

    while (!stream.eof()) {
        std::string line;
        std::stringstream lineStream;
        std::getline(stream, line);
        lineStream << line;

        std::string info;
        lineStream >> info;

        if (info == "chars") {
            uint32_t size = nextValue(lineStream);
            SPONGE_CORE_DEBUG("Reserving char map to size {}", size);
            fontChars.reserve(size);
        }

        if (info == "char") {
            uint32_t id = nextValue(lineStream);
            fontChars[id].x = nextValue(lineStream);
            fontChars[id].y = nextValue(lineStream);
            fontChars[id].width = nextValue(lineStream);
            fontChars[id].height = nextValue(lineStream);
            fontChars[id].xoffset = nextValue(lineStream);
            fontChars[id].yoffset = nextValue(lineStream);
            fontChars[id].xadvance = nextValue(lineStream);
            fontChars[id].page = nextValue(lineStream);
        }
    }

    for (const auto& [key, value] : fontChars) {
        SPONGE_CORE_DEBUG(
            "Font file: CHAR {:>3} x={:>3} y={:>3} width={:>2} height={:>3} xoffset={:2} yoffset={:3} xadvance={:>2} "
            "page={}",
            key, value.x, value.y, value.width, value.height, value.xoffset, value.yoffset, value.xadvance, value.page);
    }
}
