#pragma once

#include "core/base.h"

class Layer {
   public:
    Layer();
    virtual ~Layer() = default;

    virtual void onAttach() {}
    virtual void onDetach() {}
    virtual void onUpdate(uint32_t elapsedTime) {}
    // virtual void onEvent() {Event& event}
};
