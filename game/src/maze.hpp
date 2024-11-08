#pragma once

#include "layer/exitlayer.hpp"
#include "layer/gridlayer.hpp"
#include "layer/hudlayer.hpp"
#include "layer/imguilayer.hpp"
#include "layer/mazelayer.hpp"
#include "sponge.hpp"

namespace game {

class Maze final : public sponge::platform::sdl::core::Application {
   public:
    explicit Maze(const sponge::platform::sdl::core::ApplicationSpecification&
                      specification);

    bool onUserCreate() override;

    bool onUserDestroy() override;

    bool onUserUpdate(double elapsedTime) override;

    void onEvent(sponge::event::Event& event) override;

    static Maze& get() {
        return *instance;
    }

    std::shared_ptr<layer::MazeLayer> getMazeLayer() const {
        return mazeLayer;
    }

   private:
    bool isRunning = true;

    std::shared_ptr<layer::ExitLayer> exitLayer =
        std::make_shared<layer::ExitLayer>();
    std::shared_ptr<layer::HUDLayer> hudLayer =
        std::make_shared<layer::HUDLayer>();
    std::shared_ptr<layer::ImGuiLayer> imguiLayer =
        std::make_shared<layer::ImGuiLayer>();
    std::shared_ptr<layer::MazeLayer> mazeLayer =
        std::make_shared<layer::MazeLayer>();

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event);

    bool onWindowClose(const sponge::event::WindowCloseEvent& event);

    static Maze* instance;
};

}  // namespace game
