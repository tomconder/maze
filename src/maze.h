#ifndef INCLUDE_MAZE_H
#define INCLUDE_MAZE_H

#include "core/engine.h"
#include "core/gamecamera.h"
#include "platform/opengl/openglsprite.h"

#include <memory>
#include <string>

class Maze : public Engine
{
  public:
    Maze();

    bool onUserCreate() override;
    bool onUserUpdate(Uint32 elapsedTime) override;
    bool onUserDestroy() override;
    bool onUserResize(int width, int height) override;

  private:
    std::unique_ptr<GameCamera> camera;
    std::unique_ptr<OpenGLSprite> sprite;
};

#endif // INCLUDE_MAZE_H
