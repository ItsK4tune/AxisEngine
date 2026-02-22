#pragma once

#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <graphic/geometry/animation.h>
#include <graphic/geometry/animator.h>

struct AnimationComponent
{
    std::shared_ptr<Animation> currentAnimation = nullptr;
    std::shared_ptr<Animator> animator = nullptr;
    float currentTime = 0.0f;
    bool isPlaying = false;
    bool loop = true;
    float speed = 1.0f;

    std::vector<glm::mat4> boneMatrices;
};
