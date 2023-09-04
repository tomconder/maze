#include "core/layerstack.h"

namespace sponge {

LayerStack::~LayerStack() {
    for (auto layer : layers) {
        layer->onDetach();
    }
}
void LayerStack::pushLayer(std::shared_ptr<Layer> layer) {
    layers.emplace(layers.begin() + layerInsertIndex, layer);
    layerInsertIndex++;
}

void LayerStack::pushOverlay(std::shared_ptr<Layer> overlay) {
    layers.emplace_back(overlay);
}

void LayerStack::popLayer(std::shared_ptr<Layer> layer) {
    auto it =
        std::find(layers.begin(), layers.begin() + layerInsertIndex, layer);
    if (it != layers.begin() + layerInsertIndex) {
        layer->onDetach();
        layers.erase(it);
        layerInsertIndex--;
    }
}

void LayerStack::popOverlay(std::shared_ptr<Layer> overlay) {
    auto it =
        std::find(layers.begin() + layerInsertIndex, layers.end(), overlay);
    if (it != layers.end()) {
        overlay->onDetach();
        layers.erase(it);
    }
}

}  // namespace sponge
