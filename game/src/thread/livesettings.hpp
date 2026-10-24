#pragma once

#include "platform/opengl/scene/scenetarget.hpp"
#include "scene/light.hpp"

#include <cstdint>

namespace game::thread {

enum class AntiAliasing : uint8_t {
    None = 0,
    Fxaa,
    Taa,
    Count,
};

// The settings the debug UI and the options screens change while the game
// runs, and that the update thread copies into each MazeRenderFrame. Plain
// values only: a setting that also needs a shader uniform or a rebuild is set
// through its MazeLayer method, which edits this struct and then does the
// rest. MazeLayer guards the live copy with a mutex; read it as a whole, once
// per frame, never field by field.
struct LiveSettings {
    AntiAliasing antiAliasing = AntiAliasing::Taa;
    sponge::platform::opengl::scene::ToneMapper toneMapper =
        sponge::platform::opengl::scene::ToneMapper::Agx;

    bool  bloomEnabled   = true;
    float bloomThreshold = 0.8F;
    // Additive weight for the bloom texture, applied in LINEAR space before
    // tone mapping. The bloom texture holds radiance (measured peak ~2.6 in
    // this scene), not the [0,1] tone-mapped values it held when bloom
    // composited after the curve, so this is roughly 1/30th of the old value
    // for the same look. Re-derive it against a measurement, never by
    // scaling the old number.
    float bloomIntensity = 0.08F;

    bool  ssaoEnabled = true;
    float ssaoRadius  = 0.5F;
    bool  ssrEnabled  = true;

    bool planarEnabled = true;
    // Sample the reflection probe for ambient specular. In a frame this is
    // also false when the scene has no probe.
    bool probeEnabled = true;
    // Light diffuse ambient from the probe's irradiance. Needs probeEnabled.
    bool giEnabled = true;

    int32_t numLights        = 0;
    int32_t attenuationIndex = 4;

    game::scene::DirectionalLight directional{};

    // Cascades in use, 1 to ShadowMap::maxCascades, and the log/uniform blend
    // of their split distances.
    uint32_t shadowCascades    = 2;
    float    shadowSplitLambda = 0.7F;
};

}  // namespace game::thread
