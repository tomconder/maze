#pragma once

#include "exitlayer.h"
#include "hudlayer.h"
#include "imguilayer.h"
#include "mazelayer.h"
#include "sponge.h"
#include <memory>

class Maze : public sponge::SDLEngine {
   public:
    bool onUserCreate() override;
    bool onUserDestroy() override;
    bool onUserUpdate(double elapsedTime) override;

    void onEvent(sponge::event::Event& event) override;

   private:
    bool isRunning = true;
    bool isMouseVisible = true;

    std::shared_ptr<ExitLayer> exitLayer = std::make_shared<ExitLayer>();
    std::shared_ptr<ImGuiLayer> imguiLayer = std::make_shared<ImGuiLayer>();
    std::shared_ptr<HUDLayer> hudLayer = std::make_shared<HUDLayer>();
    std::shared_ptr<MazeLayer> mazeLayer = std::make_shared<MazeLayer>();

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event);
    bool onWindowClose(const sponge::event::WindowCloseEvent& event);
};
