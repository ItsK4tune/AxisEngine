#pragma once

#include <glm/glm.hpp>
#include <irrKlang/irrKlang.h>

class IrrKlangGLMHelpers
{
public:
    static vec3df convert(const glm::vec3 &v) { return irrklang::vec3df(v.x, v.y, v.z); }
    static glm::vec3 convert(const vec3df &v) { return glm::vec3(v.X, v.Y, v.Z); }
};