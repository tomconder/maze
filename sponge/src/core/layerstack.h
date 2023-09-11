#pragma once

#include "core/base.h"
#include "core/layer.h"

namespace sponge {

class LayerStack {
   public:
    LayerStack() = default;
    ~LayerStack();

    void pushLayer(const std::shared_ptr<Layer>& layer);
    void pushOverlay(const std::shared_ptr<Layer>& overlay);
    void popLayer(const std::shared_ptr<Layer>& layer);
    void popOverlay(const std::shared_ptr<Layer>& overlay);

    std::vector<std::shared_ptr<Layer>>::const_iterator begin() {
        return layers.begin();
    }
    std::vector<std::shared_ptr<Layer>>::const_iterator end() {
        return layers.end();
    }
    std::vector<std::shared_ptr<Layer>>::const_reverse_iterator rbegin() {
        return layers.rbegin();
    }
    std::vector<std::shared_ptr<Layer>>::const_reverse_iterator rend() {
        return layers.rend();
    }

   private:
    std::vector<std::shared_ptr<Layer>> layers;
    uint32_t layerInsertIndex = 0;
};

}  // namespace sponge
