#include "gridlayer.hpp"
#include "resourcemanager.hpp"

constexpr std::string_view gridShader = "infinitegrid";
constexpr std::string_view cameraName = "maze";
constexpr std::string_view modelName = "cube";

GridLayer::GridLayer() : Layer("grid") {
    // nothing
}

void GridLayer::onAttach() {
    sponge::renderer::OpenGLResourceManager::loadShader(
        "/shaders/infinitegrid.vert", "/shaders/infinitegrid.frag",
        gridShader.data());

    sponge::renderer::OpenGLResourceManager::loadModel(
        gridShader.data(), "/models/cube/cube.obj", modelName.data());

    camera = ResourceManager::createGameCamera(cameraName.data());
    camera->setPosition(glm::vec3(0.F, 4.F, 7.F));

    const auto shader =
        sponge::renderer::OpenGLResourceManager::getShader(gridShader.data());
    UNUSED(shader);

    grid = std::make_unique<sponge::renderer::OpenGLGrid>(gridShader.data());
}

void GridLayer::onDetach() {
    // nothing
}

bool GridLayer::onUpdate(double elapsedTime) {
    UNUSED(elapsedTime);

    auto shader =
        sponge::renderer::OpenGLResourceManager::getShader(gridShader.data());
    shader->bind();
    shader->setMat4("mvp", camera->getMVP());

    shader->unbind();

    grid->render();

    return true;
}
