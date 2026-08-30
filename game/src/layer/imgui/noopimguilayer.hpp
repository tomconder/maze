#pragma once

#include "layer/layer.hpp"

namespace game::layer::imgui {
// Inert stand-in when ENABLE_IMGUI is off, so callers need no ifdef.
class ImGuiLayer final : public sponge::layer::Layer {
public:
    ImGuiLayer() : Layer("imgui") {}
};
}  // namespace game::layer::imgui
