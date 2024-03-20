#pragma once

#include "exitlayer.hpp"
#include "hudlayer.hpp"
#include "imguilayer.hpp"
#include "mazelayer.hpp"
#include "sponge.hpp"
#include <memory>

class Maze final : public sponge::SDLEngine {
   public:
    Maze();
    bool onUserCreate() override;
    bool onUserDestroy() override;
    bool onUserUpdate(double elapsedTime) override;

    void onEvent(sponge::event::Event& event) override;

    static Maze& get() {
        return *instance;
    }

    std::shared_ptr<MazeLayer> getMazeLayer() const {
        return mazeLayer;
    }

   private:
    bool isRunning = true;
    bool isMouseVisible = true;

    std::shared_ptr<ExitLayer> exitLayer = std::make_shared<ExitLayer>();
    std::shared_ptr<ImGuiLayer> imguiLayer = std::make_shared<ImGuiLayer>();
    std::shared_ptr<HUDLayer> hudLayer = std::make_shared<HUDLayer>();
    std::shared_ptr<MazeLayer> mazeLayer = std::make_shared<MazeLayer>();

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event);
    bool onWindowClose(const sponge::event::WindowCloseEvent& event);

    static Maze* instance;
};
