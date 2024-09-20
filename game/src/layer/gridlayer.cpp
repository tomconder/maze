#include "gridlayer.hpp"
#include "resourcemanager.hpp"

const std::string gridShader{ "infinitegrid" };
const std::string cameraName{ "maze" };
const std::string modelName{ "cube" };

namespace game::layer {

using sponge::platform::opengl::renderer::ResourceManager;

GridLayer::GridLayer() : Layer("grid") {
    // nothing
}

void GridLayer::onAttach() {
    ResourceManager::loadShader("/shaders/infinitegrid.vert",
                                "/shaders/infinitegrid.frag", gridShader);

    camera = game::ResourceManager::createGameCamera(cameraName);
    camera->setPosition(glm::vec3(0.F, 4.F, 7.F));

    const auto shader = ResourceManager::getShader(gridShader);
    UNUSED(shader);

    grid =
        std::make_unique<sponge::platform::opengl::renderer::Grid>(gridShader);
}

void GridLayer::onDetach() {
    // nothing
}

bool GridLayer::onUpdate(const double elapsedTime) {
    UNUSED(elapsedTime);

    const auto shader = ResourceManager::getShader(gridShader);
    shader->bind();
    shader->setMat4("mvp", camera->getMVP());

    shader->unbind();

    grid->render();

    return true;
}

}  // namespace game::layer
