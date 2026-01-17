#include "layer/layerstack.hpp"

#include <memory>

namespace sponge::layer {
std::vector<std::shared_ptr<Layer>> layers;

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
    const auto it =
        std::find(layers.begin(), layers.begin() + layerInsertIndex, layer);
    if (it != layers.begin() + layerInsertIndex) {
        layer->onDetach();
        layers.erase(it);
        layerInsertIndex--;
    }
}

void LayerStack::popOverlay(const std::shared_ptr<Layer>& overlay) {
    const auto it =
        std::find(layers.begin() + layerInsertIndex, layers.end(), overlay);
    if (it != layers.end()) {
        overlay->onDetach();
        layers.erase(it);
    }
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
