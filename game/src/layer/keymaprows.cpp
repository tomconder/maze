#include "layer/keymaprows.hpp"

#include <cstddef>

#include "ui/menulayout.hpp"
#include "ui/rowlayout.hpp"

namespace game::layer {

void layoutKeyMapRows(const ui::MenuLayout& layout, const ui::RowText& text) {
    for (size_t row = 0; row < keyMapRowCount; row++) {
        ui::layoutBinding(layout.rowNode(row), text);
    }
}

}  // namespace game::layer
