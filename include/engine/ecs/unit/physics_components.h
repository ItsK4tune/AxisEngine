#pragma once

// Forcing recompile - 2026-04-01-T11-53-00

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <physics/interface/i_rigid_body.h>
#include <physics/interface/i_character_controller.h>
#include <physics/interface/i_constraint.h>

#define GLM_ENABLE_EXPERIMENTAL

struct RigidShapeComponent
{
    std::string type = "BOX";
    glm::vec3 size = glm::vec3(1.0f);
    float radius = 0.5f;
    float height = 1.0f;
    
    glm::vec3 offset = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    float friction = 0.5f;
    float restitution = 0.0f;
    
    // For Compound shapes
    struct ChildShape {
        std::string type;
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 size;
        float radius;
        float height;
    };
    std::vector<ChildShape> children;
};

struct RigidBodyComponent
{
    std::shared_ptr<IRigidBody> body = nullptr;

    float mass = 1.0f;
    bool isStatic = false;
    bool isKinematic = false;
    bool isTrigger = false;

    bool isAttachedToParent = false;
    bool isParentMatter = false;
    bool isChildrenMatter = false;
    bool isCollisionEnabled = true;

    std::vector<std::shared_ptr<IConstraint>> constraints;

    glm::vec3 linearFactor = glm::vec3(1.0f);
    glm::vec3 angularFactor = glm::vec3(1.0f);
    
    float linearDamping = 0.0f;
    float angularDamping = 0.0f;

    void SetRestitution(float restitution)
    {
        if (body)
            body->SetRestitution(restitution);
    }

    void SetFriction(float friction)
    {
        if (body)
            body->SetFriction(friction);
    }

    void SetLinearFactor(const glm::vec3 &factor)
    {
        linearFactor = factor;
        if (body)
            body->SetLinearFactor(factor);
    }

    void SetAngularFactor(const glm::vec3 &factor)
    {
        angularFactor = factor;
        if (body)
            body->SetAngularFactor(factor);
    }

    void SetLinearVelocity(const glm::vec3 &vel)
    {
        if (body)
        {
            body->SetLinearVelocity(vel);
            body->Activate(true);
        }
    }

    void SetAngularVelocity(const glm::vec3 &vel)
    {
        if (body)
        {
            body->SetAngularVelocity(vel);
            body->Activate(true);
        }
    }
};

struct CharacterControllerComponent
{
    std::shared_ptr<ICharacterController> controller = nullptr;

    float stepHeight = 0.35f;
    float maxSlope = 45.0f;
    
    glm::vec3 walkDirection = glm::vec3(0.0f);
    bool jumpRequested = false;
    bool isOnGround = false;
    
    void SetWorldTransform(const glm::vec3 &pos, const glm::quat &rot)
    {
        if (controller)
            controller->SetWorldTransform(pos, rot);
    }
    
    void GetWorldTransform(glm::vec3 &pos, glm::quat &rot)
    {
        if (controller)
            controller->GetWorldTransform(pos, rot);
    }
};
