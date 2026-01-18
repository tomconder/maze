#include "layer/splashscreenlayer.hpp"

#include "resourcemanager.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace {
constexpr std::string_view cameraName  = "splash";
constexpr std::string_view spriteName  = "blackcoffee-logo";
constexpr std::string_view texturePath = "textures/blackcoffee.png";

constexpr double timeoutSeconds = 5.0;
constexpr double fadeDuration   = 0.7;
constexpr float  logoSize       = 512.0F;

inline std::string spriteShaderName;
inline std::string quadShaderName;
}  // namespace

namespace game::layer {
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::Quad;
using sponge::platform::opengl::scene::Sprite;

SplashScreenLayer::SplashScreenLayer() : Layer("splash-screen") {
    spriteShaderName = Sprite::getShaderName();
    quadShaderName   = Quad::getShaderName();
}

void SplashScreenLayer::onAttach() {
    const auto orthoCameraCreateInfo =
        scene::OrthoCameraCreateInfo{ .name = std::string(cameraName) };
    orthoCamera = ResourceManager::createOrthoCamera(orthoCameraCreateInfo);

    logoSprite = std::make_unique<Sprite>(std::string(spriteName),
                                          std::string(texturePath));

    backgroundQuad = std::make_unique<Quad>();

    for (const auto& shaderName : { spriteShaderName, quadShaderName }) {
        const auto shader = AssetManager::getShader(shaderName);
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }
}

void SplashScreenLayer::onDetach() {
    // Minimal cleanup - unique_ptr handles resource deallocation
}

void SplashScreenLayer::onEvent(sponge::event::Event& event) {
    sponge::event::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::event::KeyPressedEvent>(
        [this](const sponge::event::KeyPressedEvent& event) {
            return isActive() ? this->onKeyPressed(event) : false;
        });
    dispatcher.dispatch<sponge::event::WindowResizeEvent>(
        [this](const sponge::event::WindowResizeEvent& event) {
            return this->onWindowResize(event);
        });
}

bool SplashScreenLayer::onUpdate(const double elapsedTime) {
    // Accumulate elapsed time for timeout
    elapsedTimeAccumulator += elapsedTime;
    if (elapsedTimeAccumulator >= timeoutSeconds && !isFadingFlag) {
        isFadingFlag = true;
    }

    if (isFadingFlag) {
        fadeTimeAccumulator += elapsedTime;
        currentAlpha =
            1.0F - static_cast<float>(fadeTimeAccumulator / fadeDuration);
        if (currentAlpha <= 0.0F) {
            currentAlpha      = 0.0F;
            shouldDismissFlag = true;
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

bool SplashScreenLayer::onKeyPressed(
    const sponge::event::KeyPressedEvent& event) {
    UNUSED(event);
    shouldDismissFlag = true;
    return true;
}

bool SplashScreenLayer::onWindowResize(
    const sponge::event::WindowResizeEvent& event) const {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    for (const auto& shaderName : { quadShaderName, spriteShaderName }) {
        const auto shader = AssetManager::getShader(shaderName);
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    return false;
}
}  // namespace game::layer
