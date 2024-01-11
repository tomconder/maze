#pragma once

#include "event/event.h"

namespace sponge::layer {

class Layer {
   public:
    explicit Layer(const std::string& name = "undefined");
    virtual ~Layer() = default;

    virtual void onAttach(){};
    virtual void onDetach(){};
    virtual void onEvent(event::Event& event) {}
    virtual void onImGuiRender() {}
    virtual bool onUpdate(uint32_t elapsedTime) {
        return true;
    };

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
}  // namespace sponge::layer
