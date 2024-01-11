#pragma once

#include "layer/layer.h"

namespace sponge::imgui {

class ImGuiLayer final : public layer::Layer {
   public:
    ImGuiLayer();
    ~ImGuiLayer() override = default;

    void onAttach() override;
    void onDetach() override;
    void onEvent(event::Event& event) override;

    static void begin();
    static void end();
    static void processEvent(const SDL_Event* event);
};

}  // namespace sponge::imgui
