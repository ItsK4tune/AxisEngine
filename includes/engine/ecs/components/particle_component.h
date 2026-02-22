#pragma once

#include <glm/glm.hpp>
#include <string>
#include <graphic/renderer/particle_emitter.h>

struct ParticleEmitterComponent
{
    bool isActive = true;
    ParticleEmitter emitter;
    float emissionRate = 10.0f;
    float lifetime = 2.0f;
    float speed = 1.0f;
    float size = 0.1f;
    glm::vec3 direction = glm::vec3(0.0f, 1.0f, 0.0f);
    float spread = 0.3f;
    glm::vec4 startColor = glm::vec4(1.0f);
    glm::vec4 endColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    std::string textureName = "";
};
