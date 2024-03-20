#include "layer/layerstack.hpp"

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

void LayerStack::popLayer(const std::shared_ptr<Layer>& layer) {
    auto it =
        std::find(layers.begin(), layers.begin() + layerInsertIndex, layer);
    if (it != layers.begin() + layerInsertIndex) {
        layer->onDetach();
        layers.erase(it);
        layerInsertIndex--;
    }
}

void LayerStack::popOverlay(const std::shared_ptr<Layer>& overlay) {
    auto it =
        std::find(layers.begin() + layerInsertIndex, layers.end(), overlay);
    if (it != layers.end()) {
        overlay->onDetach();
        layers.erase(it);
    }
}

}  // namespace sponge::layer
