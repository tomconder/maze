#include "platform/opengl/renderer/readback.hpp"

#include "platform/opengl/renderer/gl.hpp"

#include <array>
#include <fstream>

namespace sponge::platform::opengl::renderer {

namespace {
// A readback is a deliberate sync point, so the driver's warning that the
// pixel transfer waits for rendering is expected. The driver reports it after
// the read call returns, so muting around the call does not work: the message
// stays muted from the first readback on. Only the capture modes read back.
// A pixel pack buffer does not avoid the warning.
constexpr GLuint pixelTransferSyncWarning = 131154;

void mutePixelTransferWarning() {
    if (glDebugMessageControl != nullptr) {
        glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_PERFORMANCE,
                              GL_DONT_CARE, 1, &pixelTransferSyncWarning,
                              GL_FALSE);
    }
}
}  // namespace

Image readTexture(const uint32_t texture) {
    mutePixelTransferWarning();

    GLint width  = 0;
    GLint height = 0;
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_WIDTH, &width);
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_HEIGHT, &height);

    Image image{ static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                 std::vector<float>(static_cast<size_t>(width) *
                                    static_cast<size_t>(height) * 3U) };
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGetTextureImage(texture, 0, GL_RGB, GL_FLOAT,
                      static_cast<GLsizei>(image.rgb.size() * sizeof(float)),
                      image.rgb.data());
    return image;
}

Image readBackBuffer() {
    mutePixelTransferWarning();

    std::array<GLint, 4> viewport{};
    glGetIntegerv(GL_VIEWPORT, viewport.data());
    const auto width  = static_cast<uint32_t>(viewport[2]);
    const auto height = static_cast<uint32_t>(viewport[3]);

    std::vector<uint8_t> bytes(static_cast<size_t>(width) * height * 3U);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, static_cast<GLsizei>(width),
                 static_cast<GLsizei>(height), GL_RGB, GL_UNSIGNED_BYTE,
                 bytes.data());

    Image image{ width, height, std::vector<float>(bytes.size()) };
    for (size_t i = 0; i < bytes.size(); i++) {
        image.rgb[i] = static_cast<float>(bytes[i]) / 255.F;
    }
    return image;
}

bool writePfm(const Image& image, const char* path) {
    std::ofstream file(path, std::ios::binary);
    // Negative scale marks little endian.
    file << "PF\n" << image.width << ' ' << image.height << "\n-1.0\n";
    file.write(reinterpret_cast<const char*>(image.rgb.data()),
               static_cast<std::streamsize>(image.rgb.size() * sizeof(float)));
    return file.good();
}

}  // namespace sponge::platform::opengl::renderer
