#pragma once

#include <memory>
#include <string>

#include "gamecamera.h"
#include "hudlayer.h"
#include "mazelayer.h"
#include "sponge.h"

class Maze : public Sponge::SDLEngine {
   public:
    bool onUserCreate() override;
    bool onUserDestroy() override;
    bool onUserUpdate(Uint32 elapsedTime) override;

    void onEvent(Sponge::Event& event) override;

   private:
    bool isRunning = true;

    bool onKeyPressed(const Sponge::KeyPressedEvent& event);
    bool onWindowClose(const Sponge::WindowCloseEvent& event);
};
