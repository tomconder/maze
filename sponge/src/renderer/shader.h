#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <string>

class Shader {
   public:
    virtual ~Shader() = default;

    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    virtual void setBoolean(const std::string &name, bool value) = 0;
    virtual void setFloat(const std::string &name, float value) = 0;
    virtual void setFloat3(const std::string &name, const glm::vec3 &value) = 0;
    virtual void setInteger(const std::string &name, int value) = 0;
    virtual void setMat4(const std::string &name, const glm::mat4 &value) = 0;

    virtual const std::string &getName() const = 0;
};
