#pragma once

#include "event.h"

class EventDispatcher {
   public:
    EventDispatcher(Event& event) : event(event){};

    template <typename T, typename F>
    bool dispatch(const F& func) {
        if (event.getEventType() == T::GetStaticType()) {
            event.handled |= func(static_cast<T&>(event));
            return true;
        }
        return false;
    }

   private:
    Event& event;
};
