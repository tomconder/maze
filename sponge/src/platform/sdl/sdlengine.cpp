#include "sdlengine.h"

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

#include <SDL.h>

#include <array>
#include <glm/vec3.hpp>
#include <sstream>

int SDLEngine::construct() const {
    if (w == 0 || h == 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, appName.c_str(), "Screen height or width cannot be zero",
                                 nullptr);
        SPONGE_CORE_ERROR("Screen height or width cannot be zero");
        return 0;
    }

    return 1;
}

int SDLEngine::start() {
    SPONGE_CORE_INFO("Initializing SDL");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0) {
        SPONGE_CORE_CRITICAL("Unable to initialize SDL: {}", SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, appName.c_str(), "Unable to initialize SDL", nullptr);
        return 0;
    }

    logSDLVersion();

    SDL_Window *window = SDL_CreateWindow(appName.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    if (window == nullptr) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, appName.c_str(), "Could not create window", nullptr);
        SPONGE_CORE_CRITICAL("Could not create window: {}", SDL_GetError());
        return 0;
    }

#ifdef EMSCRIPTEN
    graphics = std::make_unique<OpenGLContext>(window, "OpenGL ES");
#else
    graphics = std::make_unique<OpenGLContext>(window, "OpenGL");
#endif

    graphics->logGlVersion();
    graphics->logStaticOpenGLInfo();
    graphics->logGraphicsDriverInfo();
    graphics->logOpenGLContextInfo();

    renderer = std::make_unique<OpenGLRendererAPI>();
    renderer->init();
    renderer->setClearColor(glm::vec4{ 0.36f, 0.36f, 0.36f, 1.0f });

    if (!onUserCreate()) {
        SDL_DestroyWindow(window);
        return 1;
    }

    lastUpdateTime = SDL_GetTicks();

    SDL_ShowWindow(window);

    return 1;
}

bool SDLEngine::iterateLoop() {
    SDL_Event event;
    uint32_t currentTime;
    uint32_t elapsedTime;

    input.beginFrame();

    bool quit = false;
    if (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            quit = true;
        } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
            adjustAspectRatio(event.window.data1, event.window.data2);
            renderer->setViewport(offsetx, offsety, w, h);
            onUserResize(w, h);
        } else if (event.type == SDL_KEYDOWN) {
            if (event.key.repeat == 0) {
                input.keyDown(event.key);
            }
        } else if (event.type == SDL_KEYUP) {
            input.keyUp(event.key);
        }

        if (event.type == SDL_MOUSEBUTTONDOWN) {
            input.mouseButtonDown(event.button);
        } else if (event.type == SDL_MOUSEBUTTONUP) {
            input.mouseButtonUp(event.button);
        } else if (event.type == SDL_MOUSEMOTION) {
            input.mouseMove(event.motion);
        } else if (event.type == SDL_MOUSEWHEEL) {
            input.mouseScroll(event.wheel);
        }
    }

    renderer->clear();

    currentTime = SDL_GetTicks();
    elapsedTime = currentTime - lastUpdateTime;
    lastUpdateTime = currentTime;

    if (!onUserUpdate(elapsedTime)) {
        quit = true;
    }

    graphics->flip();

    if (quit && onUserDestroy()) {
#ifdef EMSCRIPTEN
        emscripten_cancel_main_loop();
#endif
        SDL_Quit();

        return true;
    }

    return false;
}

void SDLEngine::logSDLVersion() {
    SDL_version compiled;
    SDL_version linked;

    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);

    std::string revision = SDL_GetRevision();
    std::stringstream ss;
    if (!revision.empty()) {
        ss << "(" << revision << ")";
    }

    SPONGE_CORE_DEBUG("SDL Version (Compiled): {}.{}.{} {}", static_cast<int>(compiled.major),
                      static_cast<int>(compiled.minor), static_cast<int>(compiled.patch),
                      !revision.empty() ? ss.str() : "");

    SPONGE_CORE_DEBUG("SDL Version (Runtime): {}.{}.{}", static_cast<int>(linked.major), static_cast<int>(linked.minor),
                      static_cast<int>(linked.patch));
}

bool SDLEngine::onUserCreate() {
    return true;
}

bool SDLEngine::onUserUpdate(uint32_t elapsedTime) {
    UNUSED(elapsedTime);
    return true;
}

bool SDLEngine::onUserResize(int width, int height) {
    UNUSED(width);
    UNUSED(height);
    return true;
}

bool SDLEngine::onUserDestroy() {
    return true;
}

void SDLEngine::adjustAspectRatio(int eventW, int eventH) {
    const std::array<glm::vec3, 5> ratios = {
        glm::vec3{ 32.f, 9.f, 32.f / 9.f },    //
        glm::vec3{ 21.f, 9.f, 21.f / 9.f },    //
        glm::vec3{ 16.f, 9.f, 16.f / 9.f },    //
        glm::vec3{ 16.f, 10.f, 16.f / 10.f },  //
        glm::vec3{ 4.f, 3.f, 4.f / 3.f }       //
    };

    // attempt to find the closest matching aspect ratio
    float proposedRatio = static_cast<float>(eventW) / static_cast<float>(eventH);
    auto exceedsRatio = [&proposedRatio](glm::vec3 i) { return proposedRatio >= i.z; };
    auto ratio = std::find_if(begin(ratios), end(ratios), exceedsRatio);
    if (ratio == std::end(ratios)) {
        --ratio;
    }

    // use ratio
    float aspectRatioWidth = ratio->x;
    float aspectRatioHeight = ratio->y;

    float aspectRatio = aspectRatioWidth / aspectRatioHeight;

    auto width = static_cast<float>(eventW);
    auto height = static_cast<float>(eventH);

    float newAspectRatio = width / height;
    if (newAspectRatio > aspectRatio) {
        w = static_cast<int>(aspectRatioWidth * height / aspectRatioHeight);
        h = eventH;
    } else {
        w = eventW;
        h = static_cast<int>(aspectRatioHeight * width / aspectRatioWidth);
    }

    offsetx = (eventW - w) / 2;
    offsety = (eventH - h) / 2;
}
