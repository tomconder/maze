#pragma once
#include <glm/vec3.hpp>

class GameObject {
   public:
    const char* name;
    const char* path;
    glm::vec3 scale{ 1.F };
    glm::vec3 translation{ 0.F };
};
