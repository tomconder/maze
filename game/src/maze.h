#pragma once

#include <memory>
#include <string>

#include "gamecamera.h"
#include "hudlayer.h"
#include "mazelayer.h"
#include "sponge.h"

struct State {
    uint16_t startWidth;
    uint16_t startHeight;
    std::string name;
};

class Maze : public Sponge::SDLEngine {
   public:
    explicit Maze(const State& state);

    bool onUserCreate() override;
    bool onUserDestroy() override;
    bool onUserUpdate(Uint32 elapsedTime) override;

    void onEvent(Sponge::Event& event) override;

   private:
    bool isRunning = true;

    bool onKeyPressed(const Sponge::KeyPressedEvent& event);
    bool onWindowClose(const Sponge::WindowCloseEvent& event);
};
