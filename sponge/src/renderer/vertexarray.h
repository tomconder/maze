#pragma once

class VertexArray {
   public:
    virtual ~VertexArray() = default;

    virtual void bind() const = 0;
};
