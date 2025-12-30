#include "scene/font.hpp"

#include "core/timer.hpp"
#include "logging/log.hpp"

#include <fmt/format.h>

#include <cerrno>
#include <charconv>
#include <fstream>
#include <ios>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace {
constexpr double secondsToMilliseconds = 1000.F;
}

namespace sponge::scene {

void Font::load(const std::string& path) {
    assert(!path.empty());

    SPONGE_CORE_DEBUG("Loading font: [{}]", path);

    core::Timer timer;
    timer.tick();

    std::ifstream stream(path, std::ios::in | std::ios::binary);
    if (!stream.good()) {
        SPONGE_CORE_ERROR("Unable to open file: {}", path);
        return;
    }

    auto unquote = [](std::string_view v) -> std::string {
        if (v.size() >= 2 && v.front() == '"' && v.back() == '"') {
            v.remove_prefix(1);
            v.remove_suffix(1);
        }
        return std::string(v);
    };

    auto parseInt = [&](std::string_view s, uint32_t& out) -> bool {
        const std::string tmp = unquote(s);
        int               val = 0;
        auto              res =
            std::from_chars(tmp.data(), tmp.data() + tmp.size(), val, 10);
        if (res.ec == std::errc{}) {
            out = static_cast<uint32_t>(val);
            return true;
        }
        return false;
    };

    auto parseFloat = [&](std::string_view s, float& out) -> bool {
        const std::string tmp    = unquote(s);
        char*             endPtr = nullptr;
        errno                    = 0;
        float val                = std::strtof(tmp.c_str(), &endPtr);
        if (errno == 0 && endPtr != tmp.c_str()) {
            out = val;
            return true;
        }
        return false;
    };

    auto parseKV = [](std::string_view line)
        -> std::unordered_map<std::string_view, std::string_view> {
        std::unordered_map<std::string_view, std::string_view> kv;

        const auto firstSpace = line.find(' ');
        if (firstSpace == std::string_view::npos) {
            return kv;
        }
        line.remove_prefix(firstSpace + 1);

        while (!line.empty()) {
            const auto start = line.find_first_not_of(' ');
            if (start == std::string_view::npos) {
                break;
            }

            line.remove_prefix(start);
            const auto end = line.find(' ');
            const auto tok = line.substr(0, end);
            const auto eq  = tok.find('=');

            if (eq != std::string_view::npos) {
                const auto key = tok.substr(0, eq);
                const auto val = tok.substr(eq + 1);
                kv.emplace(key, val);
            }

            if (end == std::string_view::npos) {
                break;
            }

            line.remove_prefix(end + 1);
        }
        return kv;
    };

    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }

        std::string tag;
        {
            std::stringstream ss(line);
            ss >> tag;
        }

        const auto kv = parseKV(line);

        if (tag == "info") {
            if (auto it = kv.find("face"); it != kv.end()) {
                face = unquote(it->second);
            }
            if (auto it = kv.find("size"); it != kv.end()) {
                float v = 0.0F;
                if (parseFloat(it->second, v)) {
                    size = v;
                }
            }
        } else if (tag == "common") {
            if (auto it = kv.find("lineHeight"); it != kv.end()) {
                parseFloat(it->second, lineHeight);
            }
            if (auto it = kv.find("base"); it != kv.end()) {
                parseFloat(it->second, base);
            }
            if (auto it = kv.find("scaleW"); it != kv.end()) {
                parseFloat(it->second, scaleW);
            }
            if (auto it = kv.find("scaleH"); it != kv.end()) {
                parseFloat(it->second, scaleH);
            }
            if (auto it = kv.find("pages"); it != kv.end()) {
                uint32_t p = 0;
                if (parseInt(it->second, p)) {
                    pages = p;
                }
            }
        } else if (tag == "page") {
            if (auto it = kv.find("file"); it != kv.end()) {
                textureName = unquote(it->second);
            }
        } else if (tag == "char") {
            uint32_t idNum = 0;
            if (auto it = kv.find("id"); it != kv.end()) {
                parseInt(it->second, idNum);
            }

            Character ch{};
            if (auto it = kv.find("x"); it != kv.end()) {
                parseFloat(it->second, ch.loc.x);
            }
            if (auto it = kv.find("y"); it != kv.end()) {
                parseFloat(it->second, ch.loc.y);
            }
            if (auto it = kv.find("width"); it != kv.end()) {
                parseFloat(it->second, ch.width);
            }
            if (auto it = kv.find("height"); it != kv.end()) {
                parseFloat(it->second, ch.height);
            }
            if (auto it = kv.find("xoffset"); it != kv.end()) {
                parseFloat(it->second, ch.offset.x);
            }
            if (auto it = kv.find("yoffset"); it != kv.end()) {
                parseFloat(it->second, ch.offset.y);
            }
            if (auto it = kv.find("xadvance"); it != kv.end()) {
                parseFloat(it->second, ch.xadvance);
            }
            if (auto it = kv.find("page"); it != kv.end()) {
                uint32_t p = 0;
                if (parseInt(it->second, p)) {
                    ch.page = p;
                }
            }

            const auto id = std::to_string(idNum);
            if (!fontChars.contains(id)) {
                fontChars.emplace(id, ch);
            }
        } else if (tag == "kerning") {
            uint32_t first  = 0;
            uint32_t second = 0;
            float    amount = 0.0F;
            if (auto it = kv.find("first"); it != kv.end()) {
                parseInt(it->second, first);
            }
            if (auto it = kv.find("second"); it != kv.end()) {
                parseInt(it->second, second);
            }
            if (auto it = kv.find("amount"); it != kv.end()) {
                parseFloat(it->second, amount);
            }

            const auto key = fmt::format("{}.{}", first, second);
            if (!kerning.contains(key)) {
                kerning.emplace(key, amount);
            }
        } else {
            SPONGE_CORE_TRACE("Unknown tag: {}", tag);
        }
    }

    timer.tick();

    SPONGE_CORE_DEBUG("Parsing time for font: {:.2f} ms",
                      timer.getElapsedSeconds() * secondsToMilliseconds);
}

void Font::log() const {
    SPONGE_CORE_DEBUG("Font {:>6} face={} size={}", "INFO", face, size);

    SPONGE_CORE_DEBUG("Font {:>6} lineHeight={:>3} base={:>3} scaleW={:>2} "
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

}  // namespace sponge::scene
