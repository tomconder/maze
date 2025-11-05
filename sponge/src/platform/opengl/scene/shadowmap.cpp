#include "platform/opengl/scene/shadowmap.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/gl.hpp"
#include "platform/opengl/renderer/resourcemanager.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <memory>

namespace {
constexpr float nearPlane = 1.F;
constexpr float farPlane = 100.F;
constexpr float orthoBoxSize = 5.F;
}  // namespace

namespace sponge::platform::opengl::scene {
ShadowMap::ShadowMap(const uint32_t res) :
    orthoSize(orthoBoxSize), shadowWidth(res), shadowHeight(res) {
    initialize();
}

void ShadowMap::initialize() {
    zNear = nearPlane;
    zFar = farPlane;

    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name = shaderName,
        .vertexShaderPath = "/shaders/shadowmap.vert.glsl",
        .fragmentShaderPath = "/shaders/shadowmap.frag.glsl",
    };
    shader = renderer::ResourceManager::createShader(shaderCreateInfo);
    shader->bind();

    const renderer::TextureCreateInfo textureCreateInfo{
        .name = "depth_map",
        .width = shadowWidth,
        .height = shadowHeight,
        .loadFlag = renderer::DepthMap
    };
    depthMap = renderer::ResourceManager::createTexture(textureCreateInfo);

    framebuffer = std::make_unique<renderer::FrameBuffer>();

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           depthMap->getId(), 0);
    if (!renderer::FrameBuffer::checkStatus()) {
        SPONGE_GL_CRITICAL("Framebuffer is not complete!");
        return;
    }

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    framebuffer->unbind();

    shader->unbind();
}

void ShadowMap::bind() const {
    glViewport(0, 0, shadowWidth, shadowHeight);
    framebuffer->bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    depthMap->activateAndBind(0);
}

void ShadowMap::unbind() const {
    framebuffer->unbind();
}

void ShadowMap::activateAndBindDepthMap(const uint8_t unit) const {
    depthMap->activateAndBind(unit);
}

std::string ShadowMap::getShaderName() {
    return std::string(shaderName);
}

uint32_t ShadowMap::getDepthMapTextureId() const {
    if (depthMap != nullptr) {
        return depthMap->getId();
    }
    return 0;
}

uint32_t ShadowMap::getHeight() const {
    return shadowHeight;
}

float ShadowMap::getOrthoSize() const {
    return orthoSize;
}

void ShadowMap::setOrthoSize(const float val) {
    orthoSize = val;
}

uint32_t ShadowMap::getWidth() const {
    return shadowWidth;
}

float ShadowMap::getZFar() const {
    return zFar;
}

void ShadowMap::setZFar(const float val) {
    zFar = val;
}

float ShadowMap::getZNear() const {
    return zNear;
}

void ShadowMap::setZNear(const float val) {
    zNear = val;
}

const glm::mat4& ShadowMap::getLightSpaceMatrix() const {
    return lightSpaceMatrix;
}

void ShadowMap::updateLightSpaceMatrix(const glm::vec3& lightDirection) {
    const float left = -orthoSize;
    const float right = orthoSize;
    const float bottom = -orthoSize;
    const float top = orthoSize;

    const auto lightProjection =
        glm::ortho(left, right, bottom, top, nearPlane, farPlane);

    const auto lightView = glm::lookAt(10.F * -lightDirection, glm::vec3(0.0f),
                                       glm::vec3(0.0f, 1.0f, 0.0f));

    lightSpaceMatrix = lightProjection * lightView;
}
}  // namespace sponge::platform::opengl::scene
