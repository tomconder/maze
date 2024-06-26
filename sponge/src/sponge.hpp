#pragma once

// for use by Sponge applications

#include "core/application.hpp"
#include "core/base.hpp"
#include "core/file.hpp"
#include "core/timer.hpp"
#include "core/window.hpp"
#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "event/keyevent.hpp"
#include "event/mouseevent.hpp"
#include "input/keycode.hpp"
#include "input/mousecode.hpp"
#include "layer/layer.hpp"
#include "layer/layerstack.hpp"
#include "logging/log.hpp"
#include "logging/logcustom.hpp"
#include "logging/logflag.hpp"
#include "platform/opengl/context.hpp"
#include "platform/opengl/font.hpp"
#include "platform/opengl/gl.hpp"
#include "platform/opengl/grid.hpp"
#include "platform/opengl/indexbuffer.hpp"
#include "platform/opengl/info.hpp"
#include "platform/opengl/mesh.hpp"
#include "platform/opengl/model.hpp"
#include "platform/opengl/quad.hpp"
#include "platform/opengl/rendererapi.hpp"
#include "platform/opengl/resourcemanager.hpp"
#include "platform/opengl/shader.hpp"
#include "platform/opengl/sprite.hpp"
#include "platform/opengl/texture.hpp"
#include "platform/opengl/vertexarray.hpp"
#include "platform/opengl/vertexbuffer.hpp"
#include "platform/sdl/application.hpp"
#include "platform/sdl/imgui/imguimanager.hpp"
#include "platform/sdl/imgui/noopmanager.hpp"
#include "platform/sdl/imgui/sdlmanager.hpp"
#include "platform/sdl/info.hpp"
#include "platform/sdl/input/keyboard.hpp"
#include "platform/sdl/input/mouse.hpp"
#include "platform/sdl/logging/sink.hpp"
#include "platform/sdl/window.hpp"
#include "renderer/buffer.hpp"
#include "renderer/font.hpp"
#include "renderer/graphicscontext.hpp"
#include "renderer/graphicsinfo.hpp"
#include "renderer/mesh.hpp"
#include "renderer/rendererapi.hpp"
#include "renderer/shader.hpp"
#include "renderer/sprite.hpp"
#include "renderer/texture.hpp"
#include "scene/camera.hpp"
#include "scene/orthocamera.hpp"
