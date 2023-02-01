#pragma once

class Buffer {
   public:
    virtual ~Buffer() = default;

    virtual void bind() const = 0;

    virtual void setData(const unsigned int *data, uint32_t size) = 0;
};
