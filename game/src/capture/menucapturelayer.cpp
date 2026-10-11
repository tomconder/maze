#include "capture/menucapturelayer.hpp"

#include "capture/capturepng.hpp"
#include "maze.hpp"
#include "platform/opengl/renderer/readback.hpp"

#include <format>
#include <utility>

namespace game::capture {
namespace {
// Long enough for the intro fade and the row scroll to finish.
constexpr double settleSeconds = 2.;
}  // namespace

MenuCaptureLayer::MenuCaptureLayer(std::string screen) :
    Layer("menucapture"), screen(std::move(screen)) {}

bool MenuCaptureLayer::onUpdate(const double elapsedTime) {
    elapsed += elapsedTime;
    if (written || elapsed < settleSeconds) {
        return true;
    }
    written = true;
    writePng(sponge::platform::opengl::renderer::readBackBuffer(),
             std::format("capture_menu_{}.png", screen).c_str());
    Maze::get().exit();
    return true;
}

}  // namespace game::capture
