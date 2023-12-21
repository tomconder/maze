#pragma once

#include "graphics/layer/layer.h"

namespace sponge::imgui {

class ImGuiLayer : public graphics::layer::Layer {
   public:
    ImGuiLayer();
    ~ImGuiLayer() override = default;

    void onAttach() override;
    void onDetach() override;
    void onEvent(event::Event& event) override;

    static void begin();
    static void end();
    static void processEvent(const SDL_Event* event);
    static void render();
};

}  // namespace sponge::imgui
