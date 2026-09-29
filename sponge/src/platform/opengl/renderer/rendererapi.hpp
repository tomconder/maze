#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace sponge::platform::opengl::renderer {

enum class DepthFunc : uint8_t { Less, LessEqual };

class RendererAPI final {
public:
    void        init() const;
    static void setViewport(int32_t x, int32_t y, int32_t width,
                            int32_t height);

    void setClearColor(const glm::vec4& color) const;
    void clear() const;

    // Clears the colour buffer of the bound framebuffer to the clear colour.
    static void clearColor();

    static void setDepth(DepthFunc func, bool write);

    // On: blend with the global default, source alpha over the destination.
    static void setAlphaBlend(bool enabled);

    static void bindTexture(uint32_t unit, uint32_t texture);
};

}  // namespace sponge::platform::opengl::renderer
