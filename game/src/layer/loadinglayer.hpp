#pragma once

#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "layer/layer.hpp"
#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/mesh.hpp"
#include "platform/opengl/scene/model.hpp"
#include "platform/opengl/scene/quad.hpp"
#include "scene/orthocamera.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

namespace game::layer {

// Shown between IntroLayer and MazeLayer while the maze's models load.
// Kicks a background thread to parse models (CPU-only: file I/O, glTF/obj
// parsing, image decode — see Model::parse()), then uploads them to the GPU
// one mesh per frame on the render thread (Model::buildMesh()) so the
// loading screen keeps animating instead of one big stall. Progress is
// tracked per mesh (not per model) so a model with many meshes — e.g.
// sponza — still moves the bar smoothly instead of appearing to freeze.
class LoadingLayer final : public sponge::layer::Layer {
public:
    LoadingLayer();

    ~LoadingLayer() override;

    void onAttach() override;

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    bool onUpdate(double elapsedTime) override;

    // Hides Layer::setActive(): activating (value == true) snapshots
    // MazeLayer's load requests and kicks off the background parse thread.
    void setActive(bool value);

private:
    std::shared_ptr<scene::OrthoCamera>                          orthoCamera;
    std::unique_ptr<sponge::platform::opengl::scene::Quad>       quad;
    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font;

    std::thread parseThread;

    std::vector<sponge::platform::opengl::scene::ModelCreateInfo> requests;
    std::vector<sponge::platform::opengl::scene::ModelData>       parsedData;
    std::vector<std::shared_ptr<sponge::platform::opengl::scene::Model>>
        builtModels;

    // Upload cursor: builds one mesh of parsedData[buildModelIndex] per
    // frame; buildingMeshes accumulates that model's built meshes until all
    // of them are ready, then becomes a Model and moves to the next one.
    std::vector<std::shared_ptr<sponge::platform::opengl::scene::Mesh>>
                buildingMeshes;
    std::size_t buildModelIndex = 0;
    std::size_t buildMeshIndex  = 0;

    // Fixed, monotonic progress: one step per mesh parsed plus one step per
    // mesh built (GPU-uploaded), sized upfront via Model::countMeshes() so
    // the bar never rescales backward and moves smoothly through models
    // with many meshes (e.g. sponza) instead of appearing to freeze.
    std::atomic<uint32_t> completedSteps{ 0 };
    uint32_t              totalSteps = 0;
    std::atomic<bool>     parseDone{ false };

    void renderProgress() const;

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;
};

}  // namespace game::layer
