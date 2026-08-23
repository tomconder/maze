#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

namespace sponge::core {

// Transparent hash functor for heterogeneous string lookup
// Allows unordered_map<string> to be searched with string_view without
// allocation
struct TransparentStringHash {
    using is_transparent = void;

    [[nodiscard]] size_t operator()(const char* str) const noexcept {
        return std::hash<std::string_view>{}(str);
    }

    [[nodiscard]] size_t operator()(const std::string_view str) const noexcept {
        return std::hash<std::string_view>{}(str);
    }

    [[nodiscard]] size_t operator()(const std::string& str) const noexcept {
        return std::hash<std::string>{}(str);
    }
};

}  // namespace sponge::core
