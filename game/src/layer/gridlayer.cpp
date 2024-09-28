#include "gridlayer.hpp"
#include "resourcemanager.hpp"

namespace {
constexpr char cameraName[] = "maze";
}  // namespace

namespace game::layer {

using sponge::platform::opengl::renderer::ResourceManager;

GridLayer::GridLayer() : Layer("grid") {
    // nothing
}

void GridLayer::onAttach() {
    grid = std::make_unique<sponge::platform::opengl::scene::Grid>();
    shaderName = sponge::platform::opengl::scene::Grid::getShaderName();

    camera = game::ResourceManager::createGameCamera(cameraName);
    camera->setPosition(glm::vec3(0.F, 4.F, 7.F));
}

void GridLayer::onDetach() {
    // nothing
}

bool GridLayer::onUpdate(const double elapsedTime) {
    UNUSED(elapsedTime);

    const auto shader = ResourceManager::getShader(shaderName);
    shader->bind();
    shader->setMat4("mvp", camera->getMVP());

    shader->unbind();

    grid->render();

    return true;
}

}  // namespace game::layer
