#pragma once

namespace sponge {

class Buffer {
   public:
    virtual ~Buffer() = default;

    virtual void bind() const = 0;
};

}  // namespace sponge
