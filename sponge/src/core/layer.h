#pragma once

#include "core/base.h"
#include "event/event.h"

namespace sponge {

class Layer {
   public:
    virtual ~Layer() = default;

    virtual bool onUpdate(uint32_t elapsedTime) = 0;
    virtual void onAttach() = 0;
    virtual void onDetach() = 0;
    virtual void onEvent(Event& event) = 0;

    bool isActive() {
        return active;
    }

    void setActive(bool value) {
        active = value;
    }

   private:
    bool active = true;
};

}  // namespace sponge
