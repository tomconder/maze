#include "layer/layerstack.hpp"

#include <memory>
#include <vector>

namespace sponge::layer {

LayerStack::~LayerStack() {
    for (const auto& layer : layers) {
        layer->onDetach();
    }
}

void LayerStack::pushLayer(const std::shared_ptr<Layer>& layer) {
    layers.emplace(layers.begin() + layerInsertIndex, layer);
    layerInsertIndex++;
}

void LayerStack::pushOverlay(const std::shared_ptr<Layer>& overlay) {
    layers.emplace_back(overlay);
}

std::vector<std::shared_ptr<Layer>>::const_iterator LayerStack::begin() {
    return layers.begin();
}

std::vector<std::shared_ptr<Layer>>::const_iterator LayerStack::end() {
    return layers.end();
}

std::vector<std::shared_ptr<Layer>>::const_reverse_iterator
    LayerStack::rbegin() {
    return layers.rbegin();
}

std::vector<std::shared_ptr<Layer>>::const_reverse_iterator LayerStack::rend() {
    return layers.rend();
}

}  // namespace sponge::layer
