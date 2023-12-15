#pragma once

#include "exitlayer.h"
#include "gamecamera.h"
#include "hudlayer.h"
#include "mazelayer.h"
#include "sponge.h"
#include <memory>
#include <string>

class Maze : public sponge::SDLEngine {
   public:
    bool onUserCreate() override;
    bool onUserDestroy() override;
    bool onUserUpdate(const uint32_t elapsedTime) override;

    void onEvent(sponge::event::Event& event) override;

   private:
    bool isRunning = true;
    bool isMouseVisible = true;

    std::shared_ptr<ExitLayer> exitLayer = std::make_shared<ExitLayer>();
    std::shared_ptr<HUDLayer> hudLayer = std::make_shared<HUDLayer>();
    std::shared_ptr<MazeLayer> mazeLayer = std::make_shared<MazeLayer>();

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event);
    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);
    bool onMouseButtonReleased(
        const sponge::event::MouseButtonReleasedEvent& event);
    bool onWindowClose(const sponge::event::WindowCloseEvent& event);
};
