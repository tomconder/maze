#pragma once

#include <memory>
#include <string>

#include "gamecamera.h"
#include "sponge.h"

class Maze : public Sponge::SDLEngine {
   public:
    Maze(int screenWidth, int screenHeight);

    bool onUserCreate() override;
    bool onUserUpdate(Uint32 elapsedTime) override;
    bool onUserDestroy() override;
    bool onUserResize(int width, int height) override;

   private:
    std::unique_ptr<GameCamera> camera;
    std::unique_ptr<Sponge::OrthoCamera> orthoCamera;
    std::unique_ptr<Sponge::OpenGLSprite> logo;
};
