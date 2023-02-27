#pragma once

#include "core/base.h"
#include "event/event.h"

namespace Sponge {

class Layer {
   public:
    Layer();
    virtual ~Layer() = default;

    virtual void onAttach() = 0;
    virtual void onDetach() = 0;
    virtual void onResize(uint32_t width, uint32_t height) = 0;
    virtual void onUpdate(uint32_t elapsedTime) = 0;

    virtual void onEvent(Event& event) {}
};

}  // namespace Sponge
