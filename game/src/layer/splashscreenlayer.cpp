#include "layer/splashscreenlayer.hpp"

#include "maze.hpp"
#include "resourcemanager.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace {
constexpr std::string_view cameraName  = "splash";
constexpr std::string_view spriteName  = "blackcoffee-logo";
constexpr std::string_view texturePath = "textures/blackcoffee.png";

constexpr double timeoutSeconds = 7.0;
constexpr double fadeDuration   = 0.7;
constexpr float  logoSize       = 512.0F;

double elapsedTimeAccumulator = 0.0;
double fadeTimeAccumulator    = 0.0;
float  currentAlpha           = 1.0F;
bool   isFadingFlag           = false;
}  // namespace

namespace game::layer {
using sponge::event::Event;
using sponge::event::EventDispatcher;
using sponge::event::MouseButtonPressedEvent;
using sponge::event::WindowResizeEvent;
using sponge::input::GameAction;
using sponge::platform::glfw::core::Application;
using sponge::platform::opengl::scene::Quad;
using sponge::platform::opengl::scene::Sprite;

SplashScreenLayer::SplashScreenLayer() : Layer("splash-screen") {}

void SplashScreenLayer::onAttach() {
    const auto orthoCameraCreateInfo =
        scene::OrthoCameraCreateInfo{ .name = std::string(cameraName) };
    orthoCamera = ResourceManager::createOrthoCamera(orthoCameraCreateInfo);

    logoSprite = std::make_unique<Sprite>(std::string(spriteName),
                                          std::string(texturePath));

    backgroundQuad = std::make_unique<Quad>();

    for (const auto& shader : { logoSprite->getShader(), Quad::getShader() }) {
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }
}

void SplashScreenLayer::onDetach() {
    // Minimal cleanup - unique_ptr handles resource deallocation
}

void SplashScreenLayer::onEvent(Event& event) {
    EventDispatcher dispatcher(event);

    dispatcher.dispatch<MouseButtonPressedEvent>(
        [this](const MouseButtonPressedEvent&) {
            if (!isActive()) {
                return false;
            }
            setActive(false);
            Maze::get().getIntroLayer()->setActive(true);
            return true;
        });
    dispatcher.dispatch<WindowResizeEvent>(
        [this](const WindowResizeEvent& event) {
            return this->onWindowResize(event);
        });
}

bool SplashScreenLayer::onUpdate(const double elapsedTime) {
    for (const auto& shader : { Quad::getShader(), logoSprite->getShader() }) {
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    auto& mgr = Application::get().getInputManager();
    mgr.setActiveContext(sponge::input::InputContext::Menu);
    const auto& snap = mgr.getSnapshot();
    if (snap.isActive(GameAction::MenuConfirm) ||
        snap.isActive(GameAction::MenuBack)) {
        mgr.consumeActive(GameAction::MenuConfirm);
        mgr.consumeActive(GameAction::MenuBack);
        setActive(false);
        Maze::get().getIntroLayer()->setActive(true);
        return true;
    }

    // Accumulate elapsed time for timeout
    elapsedTimeAccumulator += elapsedTime;
    if (elapsedTimeAccumulator >= timeoutSeconds && !isFadingFlag) {
        isFadingFlag = true;
        Maze::get().getIntroLayer()->beginFadeIn(fadeDuration);
    }

    if (isFadingFlag) {
        fadeTimeAccumulator += elapsedTime;
        currentAlpha =
            1.0F - static_cast<float>(fadeTimeAccumulator / fadeDuration);
        if (currentAlpha <= 0.0F) {
            currentAlpha = 0.0F;
            setActive(false);
        }
    }

    const auto [width, height] =
        std::pair{ static_cast<float>(orthoCamera->getWidth()),
                   static_cast<float>(orthoCamera->getHeight()) };

    backgroundQuad->render({ 0.F, 0.F }, { width, height },
                           { 0.F, 0.F, 0.F, currentAlpha });

    const auto logoPosition = calculateLogoPosition();
    logoSprite->render(logoPosition, { logoSize, logoSize }, currentAlpha);

    return true;
}

glm::vec2 SplashScreenLayer::calculateLogoPosition() const {
    const auto width  = static_cast<float>(orthoCamera->getWidth());
    const auto height = static_cast<float>(orthoCamera->getHeight());

    return { (width - logoSize) / 2.0F, (height - logoSize) / 2.0F };
}

bool SplashScreenLayer::onWindowResize(const WindowResizeEvent& event) const {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());
    return false;
}
}  // namespace game::layer
