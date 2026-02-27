#pragma once

#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <graphic/geometry/animation.h>
#include <graphic/geometry/animator.h>

struct AnimationComponent
{
    std::vector<std::string> animations;
    float speed = 1.0f;
    float startTime = 0.0f;
    float rate = 0.0f;

    std::shared_ptr<Animator> animator = nullptr;
    std::vector<glm::mat4> boneMatrices;
};
