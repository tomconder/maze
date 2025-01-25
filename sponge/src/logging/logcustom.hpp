#pragma once

#include <glm/glm.hpp>
#include <spdlog/fmt/fmt.h>

// the format() method cannot be both static and const
// ReSharper disable CppMemberFunctionMayBeStatic

template <>
struct fmt::formatter<glm::vec2> {
    constexpr auto parse(const format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const glm::vec2& vec, format_context& ctx) const
        -> decltype(ctx.out()) {
        return format_to(ctx.out(), "({:.3f}, {:.3f})", vec.x, vec.y);
    }
};

template <>
struct fmt::formatter<glm::vec3> {
    constexpr auto parse(const format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const glm::vec3& vec, format_context& ctx) const
        -> decltype(ctx.out()) {
        return format_to(ctx.out(), "({:.3f}, {:.3f}, {:.3f})", vec.x, vec.y,
                         vec.z);
    }
};

template <>
struct fmt::formatter<glm::vec4> {
    constexpr auto parse(const format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const glm::vec4& vec, format_context& ctx) const
        -> decltype(ctx.out()) {
        return format_to(ctx.out(), "({:.3f}, {:.3f}, {:.3f}, {:.3f})", vec.x,
                         vec.y, vec.z, vec.w);
    }
};

// ReSharper restore CppMemberFunctionMayBeStatic
