#pragma once

#include <memory>
#include <string>

#include "core/engine.h"
#include "gamecamera.h"
#include "platform/opengl/openglsprite.h"

class Maze : public Engine {
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
