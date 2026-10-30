#include "layer/fpslayer.hpp"

#include "core/settings.hpp"
#include "input/gameaction.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "ui/menufontsize.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <format>
#include <string>
#include <string_view>

namespace {
constexpr std::string_view fontName    = "inter";
constexpr std::string_view fontPath    = "/fonts/inter.ktx2";
constexpr std::string_view settingsKey = "video.showFps";

// The font size in pixels at 1920 wide. It snaps to a baked size.
constexpr float textSizeAt1920 = 20.F;

constexpr glm::vec3 textColor{ 1.F, 1.F, 1.F };
constexpr glm::vec4 backgroundColor{ 0.F, 0.F, 0.F, 0.5F };
}  // namespace

namespace game::layer {
using sponge::input::GameAction;
using sponge::platform::glfw::core::Application;
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::FontCreateInfo;
using sponge::platform::opengl::scene::Quad;

FpsLayer::FpsLayer() : Layer("fps") {}

void FpsLayer::onAttach() {
    font = AssetManager::createFont(FontCreateInfo{
        .name = std::string(fontName),
        .path = std::string(fontPath),
    });
    // Shared with the menus, whose resize handlers keep its size current.
    orthoCamera = scene::menuOrthoCamera();
    quad        = std::make_unique<Quad>();
    visible = sponge::core::Settings::getBool(std::string(settingsKey), false);
}

bool FpsLayer::onUpdate(const double elapsedTime) {
    if (Application::get().getInputManager().getSnapshot().isActive(
            GameAction::ToggleFps)) {
        visible = !visible;
        sponge::core::Settings::set(std::string(settingsKey), visible);
    }

    sample(elapsedTime);
    if (visible) {
        draw();
    }
    return true;
}

void FpsLayer::sample(const double elapsedTime) {
    sum -= deltas[next];
    deltas[next] = static_cast<float>(elapsedTime);
    sum += deltas[next];
    next  = (next + 1) % deltas.size();
    count = std::min(count + 1, deltas.size());
}

void FpsLayer::draw() const {
    const auto fps  = sum > 0. ? static_cast<double>(count) / sum : 0.;
    const auto text = std::format("{:.0f} FPS", fps);

    for (const auto& shader : { Quad::getShader(), font->getShader() }) {
        shader->setMat4("projection", orthoCamera->getProjection());
    }

    const auto size =
        ui::menuFontSizeForWidth(orthoCamera->getWidth(), textSizeAt1920);
    // Tabular figures keep the digits from shifting as the figure changes.
    const auto textWidth =
        static_cast<float>(font->getLength(text, size, true));
    const auto textHeight = static_cast<float>(font->getHeight(size));
    const auto margin     = static_cast<float>(size) / 2.F;

    const glm::vec2 textPosition{
        static_cast<float>(orthoCamera->getWidth()) - margin - textWidth,
        margin,
    };
    const glm::vec2 pad{ margin / 2.F, margin / 4.F };

    const glm::vec2 textSize{ textWidth, textHeight };

    quad->render(textPosition - pad, textPosition + textSize + pad,
                 backgroundColor);

    font->beginPass(size);
    font->render(text, textPosition, textColor, true);
    font->endPass();
}

}  // namespace game::layer
