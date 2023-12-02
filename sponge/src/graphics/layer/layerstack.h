#pragma once

#include "core/base.h"
#include "graphics/layer/layer.h"

namespace sponge::graphics {

class LayerStack {
   public:
    LayerStack() = default;
    ~LayerStack();

    void pushLayer(const std::shared_ptr<sponge::graphics::Layer>& layer);
    void pushOverlay(const std::shared_ptr<sponge::graphics::Layer>& overlay);
    void popLayer(const std::shared_ptr<sponge::graphics::Layer>& layer);
    void popOverlay(const std::shared_ptr<sponge::graphics::Layer>& overlay);

    std::vector<std::shared_ptr<sponge::graphics::Layer>>::const_iterator
    begin() {
        return layers.begin();
    }
    std::vector<std::shared_ptr<sponge::graphics::Layer>>::const_iterator
    end() {
        return layers.end();
    }
    std::vector<
        std::shared_ptr<sponge::graphics::Layer>>::const_reverse_iterator
    rbegin() {
        return layers.rbegin();
    }
    std::vector<
        std::shared_ptr<sponge::graphics::Layer>>::const_reverse_iterator
    rend() {
        return layers.rend();
    }

   private:
    std::vector<std::shared_ptr<sponge::graphics::Layer>> layers;
    uint32_t layerInsertIndex = 0;
};

}  // namespace sponge
