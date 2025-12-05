#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <btBulletDynamicsCommon.h>
#include <engine/graphic/model.h>
#include <engine/graphic/animator.h>

struct TransformComponent {
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 GetTransformMatrix() const;
};

struct MeshRendererComponent {
    Model* model = nullptr;
    bool castShadow = true;
};

struct RigidBodyComponent {
    btRigidBody* body = nullptr;
};

struct AnimationComponent {
    Animator* animator = nullptr;
};