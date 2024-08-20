#include "font.hpp"
#include "core/base.hpp"
#include "logging/log.hpp"
#include <fmt/format.h>
#include <cstddef>
#include <fstream>
#include <ios>
#include <sstream>

namespace sponge::renderer {

void Font::load(const std::string& path) {
    assert(!path.empty());

    std::ifstream stream(path, std::ios::in | std::ios::binary);
    if (!stream.good()) {
        SPONGE_CORE_ERROR("Unable to open file: {}", path);
        return;
    }

    auto nextInt = [](std::stringstream& sstream) {
        std::string s;
        sstream >> s;
        if (const auto pos = s.find_last_of('='); pos != std::string::npos) {
            return std::stoi(s.substr(pos + 1));
        }
        return 0;
    };

    auto nextFloat = [](std::stringstream& sstream) {
        std::string s;
        sstream >> s;
        if (const auto pos = s.find_last_of('='); pos != std::string::npos) {
            return std::stof(s.substr(pos + 1));
        }
        return 0.F;
    };

    auto nextString = [](std::stringstream& sstream) {
        std::string s;
        sstream >> s;
        if (const auto pos = s.find_last_of('='); pos != std::string::npos) {
            auto str = s.substr(pos + 1);
            // remove the surrounding quotes
            str.erase(str.begin());
            str.erase(str.end() - 1);
            return str;
        }
        return std::string{};
    };

    while (!stream.eof()) {
        std::string line;
        std::stringstream lineStream;
        std::getline(stream, line);
        lineStream << line;

        std::string str;
        lineStream >> str;

        if (str == "info") {
            face = nextString(lineStream);
            size = nextFloat(lineStream);
        }

        if (str == "common") {
            lineHeight = nextFloat(lineStream);
            base = nextFloat(lineStream);
            scaleW = nextFloat(lineStream);
            scaleH = nextFloat(lineStream);
            pages = nextInt(lineStream);
        }

        if (str == "page") {
            uint32_t id = nextInt(lineStream);
            UNUSED(id);
            std::string name = nextString(lineStream);

            textureName = name;
        }

        if (str == "char") {
            const auto id = std::to_string(nextInt(lineStream));

            Character ch;
            ch.loc = { nextFloat(lineStream), nextFloat(lineStream) };
            ch.width = nextFloat(lineStream);
            ch.height = nextFloat(lineStream);
            ch.offset = { nextFloat(lineStream), nextFloat(lineStream) };
            ch.xadvance = nextFloat(lineStream);
            ch.page = static_cast<uint32_t>(nextInt(lineStream));

            const auto iter = fontChars.find(id);
            if (iter == fontChars.end()) {
                fontChars.emplace(id, ch);
            }
        }

        if (str == "kerning") {
            uint32_t first = nextInt(lineStream);
            uint32_t second = nextInt(lineStream);
            auto key = fmt::format("{}.{}", first, second);

            float amount = nextFloat(lineStream);

            const auto iter = kerning.find(key);
            if (iter == kerning.end()) {
                kerning.emplace(key, amount);
            }
        }
    }
}

uint32_t Font::getLength(const std::string_view text,
                         const uint32_t targetSize) {
    const auto scale = static_cast<float>(targetSize) / size;
    const auto str =
        text.length() > maxLength ? text.substr(0, maxLength) : text;

    std::string prev;
    uint32_t x = 0;

    for (const char& c : str) {
        auto index = std::to_string(c);
        auto ch = fontChars[index];
        x += ch.xadvance * scale;
        if (!prev.empty()) {
            const auto key = fmt::format("{}.{}", prev, index);
            x += kerning[key] * scale;
        }
        prev = index;
    }

    return x;
}

void Font::log() const {
    SPONGE_CORE_DEBUG("Font {:>6} face={} size={}", "INFO", face, size);

    SPONGE_CORE_DEBUG(
        "Font {:>6} lineHeight={:>3} base={:>3} scaleW={:>2} "
        "scaleH={:>3} pages={:2}",
        "COMMON", lineHeight, base, scaleW, scaleH, pages);

    for (const auto& [key, value] : fontChars) {
        SPONGE_CORE_DEBUG(
            "Font {:>6} {:>3} x={:>3} y={:>3} width={:>2} height={:>3} "
            "xoffset={:2} yoffset={:3} xadvance={:>2} "
            "page={}",
            "CHAR", key, value.loc.x, value.loc.y, value.width, value.height,
            value.offset.x, value.offset.y, value.xadvance, value.page);
    }
}

}  // namespace sponge::renderer
