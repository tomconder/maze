#include "layer/layer.hpp"

#include <utility>

namespace sponge::layer {

Layer::Layer(std::string name) : debugName(std::move(name)) {
    // nothing
}

}  // namespace sponge::layer
