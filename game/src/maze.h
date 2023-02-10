#pragma once

#include <memory>
#include <string>

#include "gamecamera.h"
#include "platform/sdl/sdlengine.h"
#include "renderer/opengl/openglsprite.h"

class Maze : public SDLEngine {
   public:
    Maze(int screenWidth, int screenHeight);

    bool onUserCreate() override;
    bool onUserUpdate(Uint32 elapsedTime) override;
    bool onUserDestroy() override;
    bool onUserResize(int width, int height) override;

   private:
    std::unique_ptr<GameCamera> camera;
    std::unique_ptr<OpenGLSprite> sprite;
};
