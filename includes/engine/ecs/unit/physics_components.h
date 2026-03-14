#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <physics/interface/i_rigid_body.h>
#include <physics/interface/i_character_controller.h>
#include <physics/interface/i_constraint.h>

#define GLM_ENABLE_EXPERIMENTAL

// --- Rigid Body ---

struct RigidBodyComponent
{
    std::shared_ptr<IRigidBody> body = nullptr;

    bool isAttachedToParent = false;

    bool isParentMatter = false;
    bool isChildrenMatter = false;

    bool isCollisionEnabled = true;

    glm::vec3 positionOffset = glm::vec3(0.0f);
    glm::quat rotationOffset = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    std::vector<std::shared_ptr<IConstraint>> constraints;

    glm::vec3 linearFactor = glm::vec3(1.0f);
    glm::vec3 angularFactor = glm::vec3(1.0f);

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
        if (body)
            body->SetLinearFactor(factor);
    }

    void SetAngularFactor(const glm::vec3 &factor)
    {
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

// --- Character Controller ---

struct CharacterControllerComponent
{
    std::shared_ptr<ICharacterController> controller = nullptr;

    float stepHeight = 0.35f;
    float maxSlope = 45.0f; // degrees
    
    glm::vec3 walkDirection = glm::vec3(0.0f);
    bool jumpRequested = false;

    // Output status
    bool isOnGround = false;

    void Jump()
    {
        jumpRequested = true;
    }

    void SetWalkDirection(const glm::vec3& dir)
    {
        walkDirection = dir;
    }
};
