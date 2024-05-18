#include "gridlayer.hpp"
#include "resourcemanager.hpp"

constexpr std::string_view gridShader = "infinitegrid";
constexpr std::string_view cameraName = "maze";
constexpr std::string_view modelName = "cube";

namespace game::layer {

GridLayer::GridLayer() : Layer("grid") {
    // nothing
}

void GridLayer::onAttach() {
    sponge::platform::opengl::ResourceManager::loadShader(
        "/shaders/infinitegrid.vert", "/shaders/infinitegrid.frag",
        gridShader.data());

    camera = ResourceManager::createGameCamera(cameraName.data());
    camera->setPosition(glm::vec3(0.F, 4.F, 7.F));

    const auto shader =
        sponge::platform::opengl::ResourceManager::getShader(gridShader.data());
    UNUSED(shader);

    grid = std::make_unique<sponge::platform::opengl::Grid>(gridShader.data());
}

void GridLayer::onDetach() {
    // nothing
}

bool GridLayer::onUpdate(double elapsedTime) {
    UNUSED(elapsedTime);

    auto shader =
        sponge::platform::opengl::ResourceManager::getShader(gridShader.data());
    shader->bind();
    shader->setMat4("mvp", camera->getMVP());

    shader->unbind();

    grid->render();

    return true;
}

}  // namespace game::layer
