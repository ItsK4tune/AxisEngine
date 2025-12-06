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

struct CameraComponent {
    bool isPrimary = false;

    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    float aspectRatio = 1.0f;

    float yaw = -90.0f;
    float pitch = 0.0f;
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up    = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
    
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
};