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
    bool onUserUpdate(Uint32 elapsedTime) override;

    void onEvent(sponge::Event& event) override;

   private:
    bool isRunning = true;
    bool hasExit = false;
    ExitLayer* exitLayer = new ExitLayer();
    HUDLayer* hudLayer = new HUDLayer();
    MazeLayer* mazeLayer = new MazeLayer();

    bool onKeyPressed(const sponge::KeyPressedEvent& event);
    bool onWindowClose(const sponge::WindowCloseEvent& event);
};
