#pragma once

#include "sponge.h"

class ImGuiLayer : public sponge::graphics::layer::Layer {
   public:
    void onImGuiRender() override;
    void onAttach() override;
};
