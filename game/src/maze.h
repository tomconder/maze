#pragma once

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

    bool onKeyPressed(const sponge::KeyPressedEvent& event);
    bool onWindowClose(const sponge::WindowCloseEvent& event);
};
