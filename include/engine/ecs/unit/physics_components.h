#pragma once

#include <physics/interface/i_character_controller.h>
#include <physics/interface/i_constraint.h>
#include <physics/interface/i_rigid_body.h>
#include <physics/type/shape_type.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <string>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL

struct RigidShapeComponent
{
    ShapeType type = ShapeType::Box;
    glm::vec3 size = glm::vec3(1.0f);
    float radius = 0.5f;
    float height = 1.0f;

    glm::vec3 offset = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    float friction = 0.5f;
    float restitution = 0.0f;

    struct ChildShape
    {
        ShapeType type = ShapeType::Box;
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

    glm::vec3 initialLinearVelocity = glm::vec3(0.0f);
    glm::vec3 initialAngularVelocity = glm::vec3(0.0f);
};

inline void SetRestitution(RigidBodyComponent& rb, float restitution)
{
    if (rb.body)
        rb.body->SetRestitution(restitution);
}

inline void SetFriction(RigidBodyComponent& rb, float friction)
{
    if (rb.body)
        rb.body->SetFriction(friction);
}

inline void SetLinearFactor(RigidBodyComponent& rb, const glm::vec3& factor)
{
    rb.linearFactor = factor;
    if (rb.body)
        rb.body->SetLinearFactor(factor);
}

inline void SetAngularFactor(RigidBodyComponent& rb, const glm::vec3& factor)
{
    rb.angularFactor = factor;
    if (rb.body)
        rb.body->SetAngularFactor(factor);
}

inline void SetLinearVelocity(RigidBodyComponent& rb, const glm::vec3& vel)
{
    if (rb.body)
    {
        rb.body->SetLinearVelocity(vel);
        rb.body->Activate(true);
    }
}

inline void SetAngularVelocity(RigidBodyComponent& rb, const glm::vec3& vel)
{
    if (rb.body)
    {
        rb.body->SetAngularVelocity(vel);
        rb.body->Activate(true);
    }
}

struct CharacterControllerComponent
{
    std::shared_ptr<ICharacterController> controller = nullptr;

    float stepHeight = 0.35f;
    float maxSlope = 45.0f;

    glm::vec3 walkDirection = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    bool useVelocity = false;
    bool jumpRequested = false;
    bool isOnGround = false;
};

inline void SetWorldTransform(CharacterControllerComponent& cc, const glm::vec3& pos, const glm::quat& rot)
{
    if (cc.controller)
        cc.controller->SetWorldTransform(pos, rot);
}

inline void GetWorldTransform(CharacterControllerComponent& cc, glm::vec3& pos, glm::quat& rot)
{
    if (cc.controller)
        cc.controller->GetWorldTransform(pos, rot);
}
