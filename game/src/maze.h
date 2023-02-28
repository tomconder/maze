#pragma once

#include <memory>
#include <string>

#include "gamecamera.h"
#include "hudlayer.h"
#include "mazelayer.h"
#include "sponge.h"

class Maze : public Sponge::SDLEngine {
   public:
    Maze(int screenWidth, int screenHeight);

    bool onUserCreate() override;
    bool onUserDestroy() override;
    bool onUserUpdate(Uint32 elapsedTime) override;

    void onEvent(Sponge::Event& event) override;
    bool onKeyPressed(Sponge::KeyPressedEvent& event);
    bool onWindowClose(Sponge::WindowCloseEvent& event);

   private:
    bool isRunning = true;
};
