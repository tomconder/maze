#pragma once

#include "core/base.h"
#include "event/event.h"

namespace sponge::graphics::layer {

class Layer {
   public:
    Layer(const std::string& name = "undefined");
    virtual ~Layer() = default;

    virtual bool onUpdate(uint32_t elapsedTime) = 0;
    virtual void onAttach() = 0;
    virtual void onDetach() = 0;
    virtual void onEvent(sponge::event::Event& event) = 0;

    const std::string& getName() const {
        return debugName;
    }

    bool isActive() const {
        return active;
    }

    void setActive(bool value) {
        active = value;
    }

   protected:
    std::string debugName;

   private:
    bool active = true;
};

}  // namespace sponge::graphics::layer
