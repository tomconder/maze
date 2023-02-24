#pragma once

#include "core/base.h"
#include "layer.h"

namespace Sponge {

class LayerStack {
   public:
    LayerStack() = default;
    ~LayerStack();

    void pushLayer(Layer* layer);
    void pushOverlay(Layer* overlay);
    void popLayer(Layer* layer);
    void popOverlay(Layer* overlay);

    std::vector<Layer*>::const_iterator begin() {
        return layers.begin();
    }
    std::vector<Layer*>::const_iterator end() {
        return layers.end();
    }
    std::vector<Layer*>::const_reverse_iterator rbegin() {
        return layers.rbegin();
    }
    std::vector<Layer*>::const_reverse_iterator rend() {
        return layers.rend();
    }

   private:
    std::vector<Layer*> layers;
    uint32_t layerInsertIndex = 0;
};

}  // namespace Sponge
