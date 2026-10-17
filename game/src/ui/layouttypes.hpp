#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include <utility>

namespace game::ui {

// The size of a text in pixels.
struct TextSize {
    float width;
    float height;
};

struct Rect {
    float x;
    float y;
    float width;
    float height;
};

// Measures a text at a font size in pixels. Tabular figures give every digit
// the same width.
class TextMeasure {
public:
    TextMeasure() = default;

    template <typename Callable>
    // Implicit, like std::function: a lambda is a measure.
    TextMeasure(Callable callable) :  // NOLINT(runtime/explicit)
        measure(std::move(callable)) {}

    TextSize operator()(const std::string_view text, const uint32_t size,
                        const bool tabularFigures = false) const {
        return measure(text, size, tabularFigures);
    }

private:
    std::function<TextSize(std::string_view, uint32_t, bool)> measure;
};

}  // namespace game::ui
