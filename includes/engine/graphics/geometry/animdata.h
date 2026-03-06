#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

struct BoneInfo
{
    int id;
    glm::mat4 offset;
};
