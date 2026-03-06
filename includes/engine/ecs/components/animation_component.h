#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <graphics/geometry/animation.h>
#include <graphics/geometry/animator.h>
#include <memory>
#include <string>
#include <vector>

struct AnimationComponent
{
    std::vector<std::string> animations;
    float speed = 1.0f;
    float startTime = 0.0f;
    float rate = 0.0f;

    std::shared_ptr<Animator> animator = nullptr;
    std::vector<glm::mat4> boneMatrices;
};
