#pragma once

#include "layer/exitlayer.hpp"
#include "layer/gridlayer.hpp"
#include "layer/hudlayer.hpp"
#include "layer/imguilayer.hpp"
#include "layer/mazelayer.hpp"
#include "sponge.hpp"
#include <memory>

namespace game {
    class Maze final : public sponge::platform::sdl::Engine {
    public:
        Maze();

        bool onUserCreate() override;

        bool onUserDestroy() override;

        bool onUserUpdate(double elapsedTime) override;

        void onEvent(sponge::event::Event &event) override;

        static Maze &get() {
            return *instance;
        }

        std::shared_ptr<game::layer::MazeLayer> getMazeLayer() const {
            return mazeLayer;
        }

    private:
        bool isRunning = true;
        bool isMouseVisible = true;

        std::shared_ptr<game::layer::ExitLayer> exitLayer = std::make_shared<game::layer::ExitLayer>();
        std::shared_ptr<game::layer::GridLayer> gridLayer = std::make_shared<game::layer::GridLayer>();
        std::shared_ptr<game::layer::HUDLayer> hudLayer = std::make_shared<game::layer::HUDLayer>();
        std::shared_ptr<game::layer::ImGuiLayer> imguiLayer = std::make_shared<game::layer::ImGuiLayer>();
        std::shared_ptr<game::layer::MazeLayer> mazeLayer = std::make_shared<game::layer::MazeLayer>();

        bool onKeyPressed(const sponge::event::KeyPressedEvent &event);

        bool onWindowClose(const sponge::event::WindowCloseEvent &event);

        static Maze *instance;
    };
}
