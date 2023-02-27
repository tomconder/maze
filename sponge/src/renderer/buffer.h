#pragma once

namespace Sponge {

class Buffer {
   public:
    virtual ~Buffer() = default;

    virtual void bind() const = 0;
};

}  // namespace Sponge
