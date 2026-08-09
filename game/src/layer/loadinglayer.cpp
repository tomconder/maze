#include "layer/loadinglayer.hpp"

#include "core/base.hpp"
#include "maze.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "resourcemanager.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace {
constexpr std::string_view cameraName = "loading";
constexpr std::string_view fontName   = "inter";
constexpr std::string_view fontPath   = "/fonts/inter.ttf";

constexpr float barWidth  = 400.F;
constexpr float barHeight = 24.F;

// BitmapFont only bakes glyphs at {18, 24, 32, 48} (see
// BitmapFont::BitmapFont); getGlyph() does an exact-size lookup with no
// fallback, so this must be one of those.
constexpr uint32_t percentFontSize   = 18;
constexpr float    percentTextMargin = 12.F;

constexpr glm::vec4 backgroundColor{ 0.05F, 0.05F, 0.05F, 1.F };
constexpr glm::vec4 trackColor{ 0.2F, 0.2F, 0.2F, 1.F };
constexpr glm::vec4 fillColor{ 0.8F, 0.8F, 0.8F, 1.F };
constexpr glm::vec3 percentTextColor{ 1.F, 1.F, 1.F };
}  // namespace

namespace game::layer {
using sponge::event::Event;
using sponge::event::EventDispatcher;
using sponge::event::WindowResizeEvent;
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::FontCreateInfo;
using sponge::platform::opengl::scene::Mesh;
using sponge::platform::opengl::scene::Model;
using sponge::platform::opengl::scene::Quad;

LoadingLayer::LoadingLayer() : Layer("loading") {}

LoadingLayer::~LoadingLayer() {
    if (parseThread.joinable()) {
        parseThread.join();
    }
}

void LoadingLayer::onAttach() {
    const auto orthoCameraCreateInfo =
        scene::OrthoCameraCreateInfo{ .name = std::string(cameraName) };
    orthoCamera = ResourceManager::createOrthoCamera(orthoCameraCreateInfo);

    quad = std::make_unique<Quad>();

    const auto fontCreateInfo = FontCreateInfo{
        .name = std::string(fontName),
        .path = std::string(fontPath),
    };
    font = AssetManager::createFont(fontCreateInfo);

    for (const auto& shader : { Quad::getShader(), font->getShader() }) {
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }
}

void LoadingLayer::onDetach() {
    if (parseThread.joinable()) {
        parseThread.join();
    }
}

void LoadingLayer::onEvent(Event& event) {
    EventDispatcher dispatcher(event);

    dispatcher.dispatch<WindowResizeEvent>(
        [this](const WindowResizeEvent& resizeEvent) {
            return this->onWindowResize(resizeEvent);
        });
}

void LoadingLayer::setActive(const bool value) {
    Layer::setActive(value);
    if (!value) {
        return;
    }

    if (parseThread.joinable()) {
        parseThread.join();
    }

    requests = Maze::get().getMazeLayer()->getModelLoadRequests();
    parsedData.assign(requests.size(), {});
    builtModels.clear();
    builtModels.reserve(requests.size());
    buildingMeshes.clear();
    buildModelIndex = 0;
    buildMeshIndex  = 0;

    // Cheap structural pre-count (no vertex/image decode) so the bar can be
    // sized per-mesh up front instead of per-model — a model with many
    // meshes (sponza) would otherwise sit at one fixed value for its whole
    // parse/build instead of advancing continuously.
    std::size_t meshCount = 0;
    for (const auto& request : requests) {
        meshCount += Model::countMeshes(request);
    }
    totalSteps = static_cast<uint32_t>(2 * meshCount + 1);
    completedSteps.store(0, std::memory_order_relaxed);
    parseDone.store(false, std::memory_order_relaxed);

    parseThread = std::thread([this] {
        for (std::size_t i = 0; i < requests.size(); i++) {
            parsedData[i] = Model::parse(requests[i], [this] {
                completedSteps.fetch_add(1, std::memory_order_acq_rel);
            });
        }
        parseDone.store(true, std::memory_order_release);
    });
}

bool LoadingLayer::onUpdate(const double elapsedTime) {
    UNUSED(elapsedTime);

    renderProgress();

    if (!parseDone.load(std::memory_order_acquire)) {
        return true;
    }

    if (parseThread.joinable()) {
        parseThread.join();
    }

    if (buildModelIndex < parsedData.size()) {
        auto& meshes = parsedData[buildModelIndex].meshes;

        if (buildMeshIndex < meshes.size()) {
            buildingMeshes.emplace_back(
                Model::buildMesh(std::move(meshes[buildMeshIndex])));
            completedSteps.fetch_add(1, std::memory_order_relaxed);
            buildMeshIndex++;
            return true;
        }

        auto model = std::make_shared<Model>(std::move(buildingMeshes));
        AssetManager::registerModel(requests[buildModelIndex].name, model);
        builtModels.emplace_back(std::move(model));
        buildingMeshes.clear();
        buildModelIndex++;
        buildMeshIndex = 0;
        return true;
    }

    Maze::get().getMazeLayer()->finishLoading(std::move(builtModels));
    setActive(false);
    return true;
}

void LoadingLayer::renderProgress() const {
    for (const auto& shader : { Quad::getShader(), font->getShader() }) {
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    const auto width  = static_cast<float>(orthoCamera->getWidth());
    const auto height = static_cast<float>(orthoCamera->getHeight());

    quad->render({ 0.F, 0.F }, { width, height }, backgroundColor);

    const auto progress =
        totalSteps == 0 ?
            0.F :
            static_cast<float>(completedSteps.load(std::memory_order_acquire)) /
                static_cast<float>(totalSteps);

    const glm::vec2 barTop{ (width - barWidth) / 2.F,
                            (height - barHeight) / 2.F };

    quad->render(barTop, barTop + glm::vec2{ barWidth, barHeight }, trackColor);
    quad->render(barTop, barTop + glm::vec2{ barWidth * progress, barHeight },
                 fillColor);

    const auto percentText =
        std::to_string(static_cast<int>(progress * 100.F)) + "%";
    // Tabular figures here only: fixed-width digits keep the text from
    // shifting horizontally as the percentage changes each frame. Every
    // other use of this font (menus, status text) stays proportional.
    const auto textWidth =
        static_cast<float>(font->getLength(percentText, percentFontSize, true));
    const auto textHeight =
        static_cast<float>(font->getHeight(percentFontSize));
    const glm::vec2 textPosition{
        barTop.x - percentTextMargin - textWidth,
        barTop.y + (barHeight - textHeight) / 2.F,
    };

    font->beginPass(percentFontSize);
    font->render(percentText, textPosition, percentTextColor, true);
    font->endPass();
}

bool LoadingLayer::onWindowResize(const WindowResizeEvent& event) const {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());
    return false;
}

}  // namespace game::layer
