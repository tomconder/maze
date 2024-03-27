#pragma once

#include <spdlog/fmt/fmt.h>

namespace fmt {

template <glm::length_t L, typename Pre>
struct formatter<glm::vec<L, Pre>> : formatter<Pre> {
    auto format(const glm::vec<L, Pre>& vec, format_context& ctx) const {
        static_assert(L > 0 && L < 5, "the vec length are not supported!");

        auto out = std::copy_n("(", 1, ctx.out());
        for (glm::length_t l = 0; l < L - 1; ++l) {
            out = formatter<Pre>::format(vec[l], ctx);
            out = std::copy_n(", ", 2, out);
            ctx.advance_to(out);
        }
        out = formatter<Pre>::format(vec[L - 1], ctx);
        return std::copy_n(")", 1, out);
    }
};

}  // namespace fmt
