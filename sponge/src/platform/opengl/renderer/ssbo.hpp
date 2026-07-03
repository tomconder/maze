#pragma once

#include <cstddef>
#include <cstdint>

namespace sponge::platform::opengl::renderer {

class SSBO final {
public:
    explicit SSBO(std::size_t bytes);
    ~SSBO();

    SSBO(const SSBO&)            = delete;
    SSBO& operator=(const SSBO&) = delete;
    SSBO(SSBO&& other) noexcept;
    SSBO& operator=(SSBO&& other) noexcept;

    void update(const void* data, std::size_t bytes) const;
    void bindBase(uint32_t binding) const;

    uint32_t getId() const {
        return id;
    }

private:
    mutable uint32_t id = 0;
};

}  // namespace sponge::platform::opengl::renderer
