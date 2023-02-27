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
    virtual bool onUpdate(uint32_t elapsedTime) = 0;

    virtual void onEvent(Event& event) {}
};

}  // namespace Sponge
