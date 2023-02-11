#pragma once

#include "core/base.h"

class Layer {
   public:
    Layer();
    virtual ~Layer() = default;

    virtual void onAttach() { /* virtual */
    }
    virtual void onDetach() { /* virtual */
    }
    virtual void onUpdate(uint32_t elapsedTime) { /* virtual */
    }
    // virtual void onEvent() {Event& event}
};
