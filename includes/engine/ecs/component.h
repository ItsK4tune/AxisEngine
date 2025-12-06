#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <entt/entt.hpp>

#include <btBulletDynamicsCommon.h>
#include <engine/graphic/model.h>
#include <engine/graphic/animator.h>

#include <functional>

struct TransformComponent {
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 GetTransformMatrix() const;
};

struct MeshRendererComponent {
    Model* model = nullptr;
    Shader* shader = nullptr;
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

struct UITransformComponent {
    glm::vec2 position;
    glm::vec2 size;
    int zIndex = 0;
};

struct UIRendererComponent {
    glm::vec4 color = glm::vec4(1.0f);
    Texture* texture = nullptr;
    Shader* shader = nullptr;
};

struct UIInteractiveComponent {
    bool isHovered = false;
    bool isPressed = false;

    std::function<void(entt::entity)> onClick;
    std::function<void(entt::entity)> onHoverEnter;
    std::function<void(entt::entity)> onHoverExit;
};

struct UIAnimationComponent {
    bool isAnimating = false;
    
    float targetScale = 1.0f;
    float currentScale = 1.0f;
    float speed = 5.0f;
    
    glm::vec4 hoverColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    glm::vec4 normalColor = glm::vec4(1.0f);
};

struct DirectionalLightComponent {
    glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    
    glm::vec3 ambient = glm::vec3(0.05f);
    glm::vec3 diffuse = glm::vec3(0.4f);
    glm::vec3 specular = glm::vec3(0.5f);
};

struct PointLightComponent {
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    
    float radius = 10.0f;
};

struct SpotLightComponent {
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    
    float cutOff = glm::cos(glm::radians(12.5f));
    float outerCutOff = glm::cos(glm::radians(15.0f));
    
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
};