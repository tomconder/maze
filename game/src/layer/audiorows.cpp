#include "layer/audiorows.hpp"

#include "ui/menulayout.hpp"
#include "ui/rowlayout.hpp"

namespace game::layer {

void layoutAudioRows(const ui::MenuLayout& layout, const ui::RowText& text) {
    for (size_t row = 0; row < audioRowCount; row++) {
        ui::layoutSlider(layout.rowNode(row), text);
    }
}

}  // namespace game::layer
