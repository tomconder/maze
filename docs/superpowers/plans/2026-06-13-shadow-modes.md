# Shadow Modes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add PCSS, DPCF, and EVSM shadow filtering techniques alongside existing PCF, runtime-selectable via ImGui.

**Architecture:** Single ubershader with `uniform int shadowMode` selects which of four shadow functions runs; `ShadowMap` manages a depth texture (PCF/PCSS/DPCF) or an RG32F moment texture + two-pass Gaussian blur (EVSM), swapped on mode change. All four modes bind to the same `shadowMap` sampler2D uniform — the EVSM path reads `.rg` for moments, others read `.r` for depth.

**Tech Stack:** OpenGL 3.3 core, GLSL 3.30, GLM, existing `renderer::Shader` / `renderer::Texture` / `renderer::FrameBuffer` wrappers, Dear ImGui.

**Branch:** `feature/shadow-modes`

**Build command (Windows):**

```
cmake.exe -B build --preset windows-msvc-debug
cmake.exe --build build --target game --config Debug
```

**Run:**

```
build\maze\Debug\maze.exe
```

***

## File map

| File | Action |
|------|--------|
| `sponge/src/platform/opengl/scene/shadowmode.hpp` | Create — `ShadowMode` enum |
| `assets/shaders/glsl/shadowmap_evsm.frag.glsl` | Create — writes `(depth, depth²)` moments |
| `assets/shaders/glsl/blur.vert.glsl` | Create — fullscreen quad vert |
| `assets/shaders/glsl/blur.frag.glsl` | Create — separable Gaussian blur |
| `sponge/src/platform/opengl/scene/shadowmap.hpp` | Modify — new members/methods |
| `sponge/src/platform/opengl/scene/shadowmap.cpp` | Modify — EVSM init/destroy/blur, updated bind/unbind |
| `assets/shaders/glsl/pbr.frag.glsl` | Modify — shadowMode uniform + 3 new shadow functions |
| `game/src/thread/mazeframe.hpp` | Modify — add `shadowMode` field |
| `game/src/layer/mazelayer.hpp` | Modify — getter/setter |
| `game/src/layer/mazelayer.cpp` | Modify — wire mode through render path |
| `game/src/layer/imgui/imguilayer.cpp` | Modify — shadow mode combo |

***

### Task 1: ShadowMode enum

**Files:**

* Create: `sponge/src/platform/opengl/scene/shadowmode.hpp`

* \[ ] **Step 1: Create the file**

```cpp
#pragma once

namespace sponge::platform::opengl::scene {
enum class ShadowMode { PCF = 0, PCSS = 1, DPCF = 2, EVSM = 3 };
}
```

* \[ ] **Step 2: Build to verify it compiles**

```
cmake.exe --build build --target game --config Debug
```

Expected: successful build (no files yet include this header — zero impact).

* \[ ] **Step 3: Commit**

```
git add sponge/src/platform/opengl/scene/shadowmode.hpp
git commit -m "feat: add ShadowMode enum (PCF/PCSS/DPCF/EVSM)"
```

***

### Task 2: EVSM moment-writing fragment shader

**Files:**

* Create: `assets/shaders/glsl/shadowmap_evsm.frag.glsl`

* \[ ] **Step 1: Create the shader**

The vertex pass uses the existing `shadowmap.vert.glsl`. This fragment shader writes depth moments to the RG32F colour attachment instead of discarding.

```glsl
#version 330 core

layout(location = 0) out vec4 FragColor;

void main() {
    float depth = gl_FragCoord.z;
    FragColor = vec4(depth, depth * depth, 0.0, 1.0);
}
```

* \[ ] **Step 2: Commit**

```
git add assets/shaders/glsl/shadowmap_evsm.frag.glsl
git commit -m "feat: add EVSM moment-writing fragment shader"
```

***

### Task 3: Gaussian blur shaders

**Files:**

* Create: `assets/shaders/glsl/blur.vert.glsl`

* Create: `assets/shaders/glsl/blur.frag.glsl`

* \[ ] **Step 1: Create blur.vert.glsl**

```glsl
#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 texCoords;

void main() {
    texCoords   = aTexCoord;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
```

* \[ ] **Step 2: Create blur.frag.glsl**

Five-tap separable Gaussian. Reads RG from the source texture; writes R and G to the output (OpenGL ignores B/A for an RG32F FBO).

```glsl
#version 330 core

layout(location = 0) out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D image;
uniform bool      horizontal;

const float weight[5] =
    float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 offset = 1.0 / vec2(textureSize(image, 0));
    vec2 result = texture(image, texCoords).rg * weight[0];

    if (horizontal) {
        for (int i = 1; i < 5; ++i) {
            result +=
                texture(image, texCoords + vec2(offset.x * float(i), 0.0)).rg *
                weight[i];
            result +=
                texture(image, texCoords - vec2(offset.x * float(i), 0.0)).rg *
                weight[i];
        }
    } else {
        for (int i = 1; i < 5; ++i) {
            result +=
                texture(image, texCoords + vec2(0.0, offset.y * float(i))).rg *
                weight[i];
            result +=
                texture(image, texCoords - vec2(0.0, offset.y * float(i))).rg *
                weight[i];
        }
    }

    FragColor = vec4(result, 0.0, 1.0);
}
```

* \[ ] **Step 3: Commit**

```
git add assets/shaders/glsl/blur.vert.glsl assets/shaders/glsl/blur.frag.glsl
git commit -m "feat: add separable Gaussian blur shaders for EVSM"
```

***

### Task 4: ShadowMap header

**Files:**

* Modify: `sponge/src/platform/opengl/scene/shadowmap.hpp`

* \[ ] **Step 1: Replace the entire header**

```cpp
#pragma once

#include "platform/opengl/renderer/framebuffer.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/scene/shadowmode.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace sponge::platform::opengl::scene {
class ShadowMap {
public:
    ShadowMap() = delete;
    explicit ShadowMap(uint32_t res);
    ~ShadowMap();

    ShadowMap(const ShadowMap&)            = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    void bind() const;
    void unbind() const;

    void activateAndBindDepthMap(uint8_t unit) const;
    void activateAndBindShadowTexture(uint8_t unit) const;

    static std::string_view getShaderName() {
        return shaderName;
    }

    static std::string_view getEvsmShaderName() {
        return evsmShaderName;
    }

    uint32_t getDepthMapTextureId() const;

    uint32_t getHeight() const;

    ShadowMode getMode() const;
    void       setMode(ShadowMode mode);

    float getOrthoSize() const;
    void  setOrthoSize(float val);

    uint32_t getWidth() const;

    float getZFar() const;
    void  setZFar(float val);

    float getZNear() const;
    void  setZNear(float val);

    const glm::mat4& getLightSpaceMatrix() const;
    void             updateLightSpaceMatrix(const glm::vec3& lightDirection);

private:
    static const std::string               shaderName;
    static const std::string               evsmShaderName;
    static constexpr std::string_view      blurShaderName = "blur";

    std::shared_ptr<renderer::Shader>      shader;
    std::shared_ptr<renderer::Shader>      evsmShader;
    std::shared_ptr<renderer::Shader>      blurShader;
    std::shared_ptr<renderer::Texture>     depthMap;
    std::unique_ptr<renderer::FrameBuffer> framebuffer;

    // Raw GL handles — EVSM moment map and blur ping-pong FBOs.
    // Zero means not allocated.
    uint32_t momentTexture = 0;
    uint32_t blurTexture   = 0;
    uint32_t depthRbo      = 0;
    uint32_t momentFbo     = 0;
    uint32_t blurFbo       = 0;
    uint32_t blurVao       = 0;
    uint32_t blurVbo       = 0;

    mutable std::array<int, 4> savedViewport{};

    ShadowMode shadowMode{ ShadowMode::PCF };
    float      orthoSize;
    uint32_t   shadowHeight;
    uint32_t   shadowWidth;
    float      zFar;
    float      zNear;

    glm::mat4 lightSpaceMatrix{ 1.0f };

    void initialize();
    void initializeEvsm();
    void destroyEvsm();
    void applyBlur() const;
};
}  // namespace sponge::platform::opengl::scene
```

* \[ ] **Step 2: Build**

```
cmake.exe --build build --target game --config Debug
```

Expected: linker errors about unresolved `setMode`, `getMode`, `activateAndBindShadowTexture`, `getEvsmShaderName` — the `.cpp` hasn't been updated yet. That is expected at this stage.

* \[ ] **Step 3: Commit**

```
git add sponge/src/platform/opengl/scene/shadowmap.hpp
git commit -m "feat: extend ShadowMap header for EVSM mode support"
```

***

### Task 5: ShadowMap implementation

**Files:**

* Modify: `sponge/src/platform/opengl/scene/shadowmap.cpp`

* \[ ] **Step 1: Replace the entire file**

```cpp
#include "platform/opengl/scene/shadowmap.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <array>
#include <memory>

namespace {
constexpr float nearPlane    = 1.F;
constexpr float farPlane     = 75.F;
constexpr float orthoBoxSize = 5.F;
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

inline const std::string ShadowMap::shaderName     = "shadowmap";
inline const std::string ShadowMap::evsmShaderName = "shadowmap_evsm";

ShadowMap::ShadowMap(const uint32_t res) :
    orthoSize(orthoBoxSize),
    shadowHeight(res),
    shadowWidth(res),
    zFar(farPlane),
    zNear(nearPlane) {
    initialize();
}

ShadowMap::~ShadowMap() {
    if (shadowMode == ShadowMode::EVSM) {
        destroyEvsm();
    }
}

void ShadowMap::initialize() {
    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name               = shaderName,
        .vertexShaderPath   = "/shaders/glsl/shadowmap.vert.glsl",
        .fragmentShaderPath = "/shaders/glsl/shadowmap.frag.glsl",
    };
    shader = AssetManager::createShader(shaderCreateInfo);
    shader->bind();

    const renderer::TextureCreateInfo textureCreateInfo{
        .name     = "depth_map",
        .width    = shadowWidth,
        .height   = shadowHeight,
        .loadFlag = renderer::DepthMap
    };
    depthMap = AssetManager::createTexture(textureCreateInfo);

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

void ShadowMap::initializeEvsm() {
    // Moment texture (RG32F): R = depth, G = depth²
    glGenTextures(1, &momentTexture);
    glBindTexture(GL_TEXTURE_2D, momentTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, static_cast<GLsizei>(shadowWidth),
                 static_cast<GLsizei>(shadowHeight), 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const std::array<float, 4> borderColor = { 1.F, 1.F, 0.F, 0.F };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor.data());

    // Blur ping-pong texture (same format)
    glGenTextures(1, &blurTexture);
    glBindTexture(GL_TEXTURE_2D, blurTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, static_cast<GLsizei>(shadowWidth),
                 static_cast<GLsizei>(shadowHeight), 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Depth renderbuffer for depth testing during the moment-writing pass
    glGenRenderbuffers(1, &depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                          static_cast<GLsizei>(shadowWidth),
                          static_cast<GLsizei>(shadowHeight));

    // Moment FBO: colour = momentTexture, depth = depthRbo
    glGenFramebuffers(1, &momentFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, momentFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           momentTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depthRbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SPONGE_GL_CRITICAL("EVSM moment framebuffer is not complete!");
    }

    // Blur FBO: colour = blurTexture (no depth needed)
    glGenFramebuffers(1, &blurFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, blurFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           blurTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SPONGE_GL_CRITICAL("EVSM blur framebuffer is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Fullscreen quad VAO/VBO for blur pass
    constexpr std::array<float, 24> quadVerts = {
        -1.F,  1.F, 0.F, 1.F,
        -1.F, -1.F, 0.F, 0.F,
         1.F, -1.F, 1.F, 0.F,
        -1.F,  1.F, 0.F, 1.F,
         1.F, -1.F, 1.F, 0.F,
         1.F,  1.F, 1.F, 1.F
    };
    glGenVertexArrays(1, &blurVao);
    glGenBuffers(1, &blurVbo);
    glBindVertexArray(blurVao);
    glBindBuffer(GL_ARRAY_BUFFER, blurVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);

    // EVSM moment-writing shader (reuses shadowmap.vert.glsl)
    const auto evsmShaderInfo = renderer::ShaderCreateInfo{
        .name               = evsmShaderName,
        .vertexShaderPath   = "/shaders/glsl/shadowmap.vert.glsl",
        .fragmentShaderPath = "/shaders/glsl/shadowmap_evsm.frag.glsl",
    };
    evsmShader = AssetManager::createShader(evsmShaderInfo);

    // Gaussian blur shader
    const auto blurShaderInfo = renderer::ShaderCreateInfo{
        .name               = std::string(blurShaderName),
        .vertexShaderPath   = "/shaders/glsl/blur.vert.glsl",
        .fragmentShaderPath = "/shaders/glsl/blur.frag.glsl",
    };
    blurShader = AssetManager::createShader(blurShaderInfo);
}

void ShadowMap::destroyEvsm() {
    if (blurVao != 0) {
        glDeleteVertexArrays(1, &blurVao);
        blurVao = 0;
    }
    if (blurVbo != 0) {
        glDeleteBuffers(1, &blurVbo);
        blurVbo = 0;
    }
    if (blurFbo != 0) {
        glDeleteFramebuffers(1, &blurFbo);
        blurFbo = 0;
    }
    if (momentFbo != 0) {
        glDeleteFramebuffers(1, &momentFbo);
        momentFbo = 0;
    }
    if (blurTexture != 0) {
        glDeleteTextures(1, &blurTexture);
        blurTexture = 0;
    }
    if (momentTexture != 0) {
        glDeleteTextures(1, &momentTexture);
        momentTexture = 0;
    }
    if (depthRbo != 0) {
        glDeleteRenderbuffers(1, &depthRbo);
        depthRbo = 0;
    }
    evsmShader = nullptr;
    blurShader = nullptr;
}

void ShadowMap::applyBlur() const {
    glDisable(GL_DEPTH_TEST);
    blurShader->bind();
    blurShader->setInteger("image", 0);
    glBindVertexArray(blurVao);
    glActiveTexture(GL_TEXTURE0);

    // Horizontal pass: read momentTexture → write blurTexture
    glBindFramebuffer(GL_FRAMEBUFFER, blurFbo);
    blurShader->setBoolean("horizontal", true);
    glBindTexture(GL_TEXTURE_2D, momentTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Vertical pass: read blurTexture → write momentTexture
    glBindFramebuffer(GL_FRAMEBUFFER, momentFbo);
    blurShader->setBoolean("horizontal", false);
    glBindTexture(GL_TEXTURE_2D, blurTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    blurShader->unbind();
    glEnable(GL_DEPTH_TEST);
}

void ShadowMap::bind() const {
    glGetIntegerv(GL_VIEWPORT, savedViewport.data());
    glViewport(0, 0, static_cast<GLsizei>(shadowWidth),
               static_cast<GLsizei>(shadowHeight));
    if (shadowMode == ShadowMode::EVSM) {
        glBindFramebuffer(GL_FRAMEBUFFER, momentFbo);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        framebuffer->bind();
        glClear(GL_DEPTH_BUFFER_BIT);
        depthMap->activateAndBind(0);
    }
}

void ShadowMap::unbind() const {
    if (shadowMode == ShadowMode::EVSM) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        applyBlur();
    } else {
        framebuffer->unbind();
    }
    glViewport(savedViewport[0], savedViewport[1],
               static_cast<GLsizei>(savedViewport[2]),
               static_cast<GLsizei>(savedViewport[3]));
}

void ShadowMap::activateAndBindDepthMap(const uint8_t unit) const {
    depthMap->activateAndBind(unit);
}

void ShadowMap::activateAndBindShadowTexture(const uint8_t unit) const {
    if (shadowMode == ShadowMode::EVSM) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, momentTexture);
    } else {
        depthMap->activateAndBind(unit);
    }
}

uint32_t ShadowMap::getDepthMapTextureId() const {
    if (shadowMode == ShadowMode::EVSM) {
        return momentTexture;
    }
    if (depthMap != nullptr) {
        return depthMap->getId();
    }
    return 0;
}

uint32_t ShadowMap::getHeight() const {
    return shadowHeight;
}

ShadowMode ShadowMap::getMode() const {
    return shadowMode;
}

void ShadowMap::setMode(const ShadowMode mode) {
    if (shadowMode == mode) {
        return;
    }
    if (shadowMode == ShadowMode::EVSM) {
        destroyEvsm();
    }
    shadowMode = mode;
    if (shadowMode == ShadowMode::EVSM) {
        initializeEvsm();
    }
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
    const float left   = -orthoSize;
    const float right  = orthoSize;
    const float bottom = -orthoSize;
    const float top    = orthoSize;

    const auto lightProjection =
        glm::ortho(left, right, bottom, top, zNear, zFar);

    const auto lightView = glm::lookAt(10.F * -lightDirection, glm::vec3(0.0f),
                                       glm::vec3(0.0f, 1.0f, 0.0f));

    lightSpaceMatrix = lightProjection * lightView;
}
}  // namespace sponge::platform::opengl::scene
```

* \[ ] **Step 2: Build**

```
cmake.exe --build build --target game --config Debug
```

Expected: successful build (no callers of the new methods yet).

* \[ ] **Step 3: Commit**

```
git add sponge/src/platform/opengl/scene/shadowmap.cpp
git commit -m "feat: implement EVSM mode in ShadowMap (init, blur, bind/unbind)"
```

***

### Task 6: PBR fragment shader — shadow functions

**Files:**

* Modify: `assets/shaders/glsl/pbr.frag.glsl`

* \[ ] **Step 1: Add uniforms after the existing sampler2D declarations**

Find the block (around line 37–38):

```glsl
uniform sampler2D texture_diffuse1;
uniform sampler2D shadowMap;
```

Add after `uniform sampler2D shadowMap;`:

```glsl
uniform int   shadowMode;         // 0=PCF 1=PCSS 2=DPCF 3=EVSM
uniform float pcssLightSize;      // light size for PCSS blocker search
uniform float pcfRadius;          // disk radius for DPCF
uniform float evsmBleedThreshold; // light bleed clamp for EVSM
```

* \[ ] **Step 2: Update function prototypes (around line 56–57)**

Replace:

```glsl
float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir,
                      float shadowBias);
```

With:

```glsl
float shadowPCF(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir,
                float shadowBias);
float shadowPCSS(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir,
                 float shadowBias);
float shadowDPCF(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir,
                 float shadowBias);
float shadowEVSM(vec4 fragPosLightSpace);
float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir,
                      float shadowBias);
```

* \[ ] **Step 3: Update the calculateShadow call in main() (around line 82)**

The call site is unchanged — `calculateShadow` still dispatches. No edit needed here.

* \[ ] **Step 4: Replace the calculateShadow implementation and add new functions**

Replace the entire `calculateShadow` function (starting at line 153) and everything after the PBR math functions with:

```glsl
// Poisson disk for DPCF (16 samples)
const vec2 poissonDisk[16] = vec2[16](
    vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790));

float shadowPCF(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir,
                float shadowBias) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords      = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0 || projCoords.z < 0.0) {
        return 0.0;
    }
    float bias         = max(shadowBias * (1.0 - dot(normal, lightDir)), 0.005);
    float currentDepth = projCoords.z - bias;
    float shadow       = 0.0;
    vec2  texelSize    = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float d =
                texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth > d ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

float findBlockerDistance(vec2 uv, float currentDepth, float searchWidth) {
    float blockerSum   = 0.0;
    int   blockerCount = 0;
    vec2  texelSize    = 1.0 / textureSize(shadowMap, 0);
    for (int x = -2; x <= 1; ++x) {
        for (int y = -2; y <= 1; ++y) {
            float d =
                texture(shadowMap, uv + vec2(x, y) * texelSize * searchWidth).r;
            if (d < currentDepth) {
                blockerSum += d;
                ++blockerCount;
            }
        }
    }
    return blockerCount == 0 ? -1.0 : blockerSum / float(blockerCount);
}

float shadowPCSS(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir,
                 float shadowBias) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords      = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0 || projCoords.z < 0.0) {
        return 0.0;
    }
    float bias         = max(shadowBias * (1.0 - dot(normal, lightDir)), 0.005);
    float currentDepth = projCoords.z - bias;

    float avgBlocker =
        findBlockerDistance(projCoords.xy, currentDepth, pcssLightSize * 10.0);
    if (avgBlocker < 0.0) {
        return 0.0;
    }

    float penumbra  = (currentDepth - avgBlocker) / avgBlocker * pcssLightSize;
    float shadow    = 0.0;
    vec2  texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float d = texture(shadowMap,
                              projCoords.xy +
                                  vec2(x, y) * texelSize * penumbra * 10.0)
                          .r;
            shadow += currentDepth > d ? 1.0 : 0.0;
        }
    }
    return shadow / 25.0;
}

float shadowDPCF(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir,
                 float shadowBias) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords      = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0 || projCoords.z < 0.0) {
        return 0.0;
    }
    float bias         = max(shadowBias * (1.0 - dot(normal, lightDir)), 0.005);
    float currentDepth = projCoords.z - bias;
    float shadow       = 0.0;
    vec2  texelSize    = 1.0 / textureSize(shadowMap, 0);
    for (int i = 0; i < 16; ++i) {
        float d = texture(shadowMap,
                          projCoords.xy + poissonDisk[i] * texelSize * pcfRadius)
                      .r;
        shadow += currentDepth > d ? 1.0 : 0.0;
    }
    return shadow / 16.0;
}

float shadowEVSM(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords      = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0 || projCoords.z < 0.0) {
        return 0.0;
    }
    float currentDepth = projCoords.z;
    vec2  moments      = texture(shadowMap, projCoords.xy).rg;

    float p        = float(currentDepth <= moments.x);
    float variance = moments.y - moments.x * moments.x;
    variance       = max(variance, 0.00002);
    float d        = currentDepth - moments.x;
    float pMax     = variance / (variance + d * d);
    pMax = clamp((pMax - evsmBleedThreshold) / (1.0 - evsmBleedThreshold), 0.0,
                 1.0);
    return 1.0 - max(p, pMax);
}

float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir,
                      float shadowBias) {
    if (shadowMode == 1) {
        return shadowPCSS(fragPosLightSpace, normal, lightDir, shadowBias);
    }
    if (shadowMode == 2) {
        return shadowDPCF(fragPosLightSpace, normal, lightDir, shadowBias);
    }
    if (shadowMode == 3) {
        return shadowEVSM(fragPosLightSpace);
    }
    return shadowPCF(fragPosLightSpace, normal, lightDir, shadowBias);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 +
           (1.0 - F0) * pow(2.0, (-5.55473 * cosTheta - 6.98316) * cosTheta);
}

float distributionGGX(vec3 N, vec3 H, float rough) {
    float a      = rough * rough;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float nom    = a2;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom        = M_PI * denom * denom;
    return nom / denom;
}

float geometrySchlickGGX(float NdotV, float rough) {
    float r   = (rough + 1.0);
    float k   = (r * r) / 8.0;
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float geometrySmith(float NdotV, float NdotL, float rough) {
    float ggx2 = geometrySchlickGGX(NdotV, rough);
    float ggx1 = geometrySchlickGGX(NdotL, rough);
    return ggx1 * ggx2;
}
```

* \[ ] **Step 5: Build**

```
cmake.exe --build build --target game --config Debug
```

Expected: successful build (new uniforms have no value set yet — they'll default to 0.0, which means PCF mode runs and bias/radius uniforms are zeroed, which is suboptimal but non-crashing).

* \[ ] **Step 6: Commit**

```
git add assets/shaders/glsl/pbr.frag.glsl
git commit -m "feat: add PCSS, DPCF, EVSM shadow functions to PBR shader"
```

***

### Task 7: Wire shadow mode through MazeRenderFrame and MazeLayer

**Files:**

* Modify: `game/src/thread/mazeframe.hpp`

* Modify: `game/src/layer/mazelayer.hpp`

* Modify: `game/src/layer/mazelayer.cpp`

* \[ ] **Step 1: Add `shadowMode` to MazeRenderFrame**

In `game/src/thread/mazeframe.hpp`, add the include at the top after the existing includes:

```cpp
#include "platform/opengl/scene/shadowmode.hpp"
```

In the struct body, after `bool shadowCastShadow{ false };`:

```cpp
sponge::platform::opengl::scene::ShadowMode shadowMode{
    sponge::platform::opengl::scene::ShadowMode::PCF
};
```

* \[ ] **Step 2: Add to MazeLayer header**

In `game/src/layer/mazelayer.hpp`, add the include at the top after the existing `#include "sponge.hpp"`:

```cpp
#include "platform/opengl/scene/shadowmode.hpp"
```

Add these two method declarations in the public section after `setDirectionalLightShadowBias`:

```cpp
sponge::platform::opengl::scene::ShadowMode getShadowMode() const;
void setShadowMode(sponge::platform::opengl::scene::ShadowMode mode);
```

Add this private member after `bool mouseButtonPressed = false;`:

```cpp
sponge::platform::opengl::scene::ShadowMode shadowMode{
    sponge::platform::opengl::scene::ShadowMode::PCF
};
```

* \[ ] **Step 3: Implement getShadowMode / setShadowMode in mazelayer.cpp**

Add this include at the top of `mazelayer.cpp` after the existing includes:

```cpp
#include "platform/opengl/scene/shadowmode.hpp"
```

Add these two functions after `setDirectionalLightShadowBias`:

```cpp
sponge::platform::opengl::scene::ShadowMode MazeLayer::getShadowMode() const {
    return shadowMode;
}

void MazeLayer::setShadowMode(
    const sponge::platform::opengl::scene::ShadowMode mode) {
    shadowMode = mode;
    shadowMap->setMode(mode);
}
```

* \[ ] **Step 4: Set PBR shader uniforms for new shadow params in `onAttach()`**

In `mazelayer.cpp`, in `onAttach()`, after `shader->setFloat("directionalLight.shadowBias", ...)` and before `shader->unbind()`:

```cpp
shader->setInteger("shadowMode", 0);
shader->setFloat("pcssLightSize", 5.F);
shader->setFloat("pcfRadius", 3.F);
shader->setFloat("evsmBleedThreshold", 0.2F);
```

* \[ ] **Step 5: Capture shadowMode in captureRenderFrame()**

In `captureRenderFrame()`, after `frame.shadowCastShadow = directionalLight.castShadow;`:

```cpp
frame.shadowMode = shadowMode;
```

* \[ ] **Step 6: Use activateAndBindShadowTexture and set shadowMode uniform in renderGameObjects()**

In `renderGameObjects()`, find:

```cpp
shader->setInteger("shadowMap", 1);
shadowMap->activateAndBindDepthMap(1);
```

Replace with:

```cpp
shader->setInteger("shadowMap", 1);
shader->setInteger("shadowMode", static_cast<int>(frame.shadowMode));
shadowMap->activateAndBindShadowTexture(1);
```

* \[ ] **Step 7: Use EVSM shader in renderSceneToDepthMap()**

In `renderSceneToDepthMap()`, find:

```cpp
const auto shader = AssetManager::getShader(ShadowMap::getShaderName());
```

Replace with:

```cpp
const auto& activeShaderName =
    frame.shadowMode == sponge::platform::opengl::scene::ShadowMode::EVSM
        ? ShadowMap::getEvsmShaderName()
        : ShadowMap::getShaderName();
const auto shader = AssetManager::getShader(activeShaderName);
```

* \[ ] **Step 8: Build**

```
cmake.exe --build build --target game --config Debug
```

Expected: successful build with no warnings about unused variables.

* \[ ] **Step 9: Run and verify PCF still works**

```
build\maze\Debug\maze.exe
```

Expected: app launches, shadows render identically to before (PCF is the default mode, no ImGui switch yet).

* \[ ] **Step 10: Commit**

```
git add game/src/thread/mazeframe.hpp game/src/layer/mazelayer.hpp game/src/layer/mazelayer.cpp
git commit -m "feat: wire ShadowMode through MazeRenderFrame and MazeLayer"
```

***

### Task 8: ImGui shadow mode dropdown

**Files:**

* Modify: `game/src/layer/imgui/imguilayer.cpp`

* \[ ] **Step 1: Add the include**

At the top of `imguilayer.cpp`, after existing includes:

```cpp
#include "platform/opengl/scene/shadowmode.hpp"
```

* \[ ] **Step 2: Add Shadow Mode row to the directional lights table**

In `imguilayer.cpp`, in the directional lights section, find the `showTableRow` block for `"Cast Shadow"` (around line 226). Insert a new `showTableRow` immediately **before** it:

```cpp
showTableRow([&] {
    ImGui::Text("Shadow Mode");
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    const char* shadowModeNames[] = { "PCF", "PCSS", "DPCF", "EVSM" };
    auto        currentMode =
        static_cast<int>(mazeLayer->getShadowMode());
    if (ImGui::Combo("##shadowmode", &currentMode, shadowModeNames, 4)) {
        mazeLayer->setShadowMode(
            static_cast<sponge::platform::opengl::scene::ShadowMode>(
                currentMode));
    }
});
```

* \[ ] **Step 3: Build**

```
cmake.exe --build build --target game --config Debug
```

Expected: successful build, no errors.

* \[ ] **Step 4: Run and exercise all four modes**

```
build\maze\Debug\maze.exe
```

Open ImGui (press the debug UI key), expand the Directional Lights section, and test each shadow mode:

* **PCF**: Hard-edged, aliased shadows — same as before.
* **DPCF**: Soft, noise-free shadows with a fixed penumbra. No contact hardening.
* **PCSS**: Soft shadows with contact hardening — umbra narrows near the caster.
* **EVSM**: Smooth, filtered shadows with no acne. May show light bleeding on large occluders (tunable via the `evsmBleedThreshold` default of 0.2).

Verify: switching modes doesn't crash, shadow map preview in ImGui updates (shows moment texture for EVSM).

* \[ ] **Step 5: Commit**

```
git add game/src/layer/imgui/imguilayer.cpp
git commit -m "feat: add shadow mode dropdown to ImGui panel"
```

***

### Task 9: Final verification

* \[ ] **Step 1: Release build**

```
cmake.exe -B build --preset windows-msvc-release
cmake.exe --build build --target game --config Release
build\maze\Release\maze.exe
```

Expected: no crashes, all four shadow modes render correctly at release optimisation level.

* \[ ] **Step 2: Cycle through all modes and verify no GL errors**

Switch PCF → PCSS → DPCF → EVSM → PCF. Confirm in the application log that no GL errors or critical messages appear.

* \[ ] **Step 3: Final commit if any fixups were needed**

```
git add -p
git commit -m "fix: shadow mode cleanup from release build verification"
```
