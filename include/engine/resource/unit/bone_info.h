#pragma once

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL

struct BoneInfo
{
    int id;
    glm::mat4 offset;
};
